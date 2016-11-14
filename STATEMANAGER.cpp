#include "STATEMANAGER.h"
#include "TUPLE.h"
#include <stdlib.h>
#include <string.h>
#include "Debug.h"

STATEMANAGER::STATEMANAGER ( char* uesr, char* password, char* db_name )
{
    strcpy ( this->user, uesr );
    strcpy ( this->password, password );
    this->port = 3036;
    strcpy( this->db_name, db_name );
}

DBCONN* STATEMANAGER::getconn (unsigned int uuid)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    DBCONN* conn = new DBCONN;

    mysql_init ( &conn->connection );

    if ( !mysql_real_connect ( &conn->connection, "localhost", this->user, this->password, NULL, this->port, NULL, 0 ) )
    {
        printf ( "connect error!\n" );
        exit ( 0 );
    }
    
    mysql_select_db ( &conn->connection, this->db_name );
    
    conn->uuid = uuid;

    this->conns.push_back ( conn );

    return conn;
}

void STATEMANAGER::writeTable ( DBCONN* conn, char* table_name, int key_value, double value, int column )
{
    char buffer[1000] = "";
    MYSQL_RES *res;
    MYSQL_ROW row;

    sprintf ( buffer, "SELECT `COLUMN_NAME` FROM `INFORMATION_SCHEMA`.`COLUMNS` WHERE `TABLE_SCHEMA`='edge1' AND `TABLE_NAME`='%s'", table_name );
    if ( mysql_query ( &conn->connection, buffer ) != 0 )
    {
        printf ( "query error in %s error: %s\n", __func__, mysql_error ( &conn->connection ) );
        exit ( 0 );
    }

    char key_column[10] ="";
    char target_column[10] ="";
    res = mysql_store_result ( &conn->connection );

    for ( int i = 0; ; i++ )
    {
        row = mysql_fetch_row ( res );
        if ( row == NULL )
            break;

        // key column
        if ( i ==  0 )
        {
            strcpy ( key_column, row[0] );
        }

        if ( i == column )
        {
            strcpy ( target_column, row[0] );
            break;
        }
    }


    sprintf ( buffer, "update %s set %s='%f' where %s.%s = %d ", table_name, target_column, value, table_name, key_column, key_value );
    if ( mysql_query ( &conn->connection, buffer ) != 0 )
    {
        printf ( "query error in %s error: %s\n", __func__, mysql_error ( &conn->connection ) );
        exit ( 0 );
    }
    int num_fields = mysql_affected_rows ( &conn->connection );

    if ( num_fields == 0 )
    {
        printf ( "no fields are found, data insert!\n" );
        sprintf ( buffer, "insert into %s.%s (%s, %s) values ('%d', '%f')", this->db_name, table_name, key_column, target_column, key_value, value );
        if ( mysql_query ( &conn->connection, buffer ) != 0 )
        {
            printf ( "query error in %s error: %s\n", __func__, mysql_error ( &conn->connection ) );
            exit ( 0 );
        }
    }
}

TUPLE* STATEMANAGER::readTable ( DBCONN* conn, char* table_name, int key_value )
{
    char buffer[200] = "";
    char temp[3] = "";
    MYSQL_RES *res_values;
    MYSQL_ROW row_values;
    sprintf ( temp, "%d", key_value );

    sprintf ( buffer, "SELECT * FROM `%s` WHERE `ip`='%s'", table_name, temp );
    mysql_query ( &conn->connection, buffer );
    res_values = mysql_store_result ( &conn->connection );
    if ( res_values == NULL )
    {
        printf ( "no data in db!\n" );
        return NULL;
    }
    row_values = mysql_fetch_row ( res_values );
    int fields_values = mysql_num_fields ( res_values );

    TUPLE *tuple = new TUPLE ( 200, 0, 0 );

    for ( int i = 0; i < fields_values; i++ )
    {
        float value = ( float ) atof ( row_values[i] );
        tuple->push ( &value, sizeof ( float ) );
    }

    tuple->sealing();

    return tuple;
}

void STATEMANAGER::migrateDB ( int sockfd, unsigned int uuid )
{
    //SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE = 'BASE TABLE' AND TABLE_SCHEMA='edge1'
    char buffer[300] = "";
    
    TUPLE* tuple = new TUPLE ( 4096, 0, 0x04 );
    
    MYSQL_RES *res;
    MYSQL_ROW row;
    
    struct DBCONN* conn = NULL;
    for(auto iter = conns.begin(); iter != conns.end(); ++iter)
    {
        struct DBCONN* conn_temp = *iter;
        if(conn_temp->uuid == uuid)
        {
            conn = conn_temp;
            break;
        }
    }
    
    if(conn == NULL)
    {
        printf("no db connection for uuid: %d\n", uuid);
        exit(0);
    }
    
    sprintf(buffer, "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE = 'BASE TABLE' AND TABLE_SCHEMA='%s'", this->db_name);
    
    if ( mysql_query ( &conn->connection, buffer ) != 0 )
    {
        printf ( "query error in %s error: %s\n", __func__, mysql_error ( &conn->connection ) );
        exit ( 0 );
    }
    
    res = mysql_store_result ( &conn->connection );
    
    while(row = mysql_fetch_row(res))
    {
        // 각 테이블마다의 튜플
        // 0xaa 0xsize 0x05 (name size 1byte) (table_name n byte) (rows short) (cols short) (4byte for each field) ....  
        TUPLE* table_tuple = new TUPLE(256, 0, 0x05);

        // table 명이 row[0]에 찍힌다.
        char* table_name = row[0];
        char size = strlen(table_name);
        table_tuple->push(&size, 1);
        table_tuple->push(table_name, strlen(table_name));
        
        // for each table, fetch using uuid
        sprintf(buffer, "SELECT * FROM `%s` WHERE `ip`='%d'", table_name, uuid);
        if(mysql_query( &conn->connection, buffer ) != 0 )
        {
            printf ( "query error in %s error: %s\n", __func__, mysql_error ( &conn->connection ) );
            exit ( 0 );
        }
        MYSQL_RES *res_table;
        MYSQL_ROW row_table;
        
        res_table = mysql_store_result ( &conn->connection );
        short rows = mysql_num_rows ( res_table );
        short cols = mysql_num_fields ( res_table );
        
        table_tuple->push(&rows, sizeof rows);
        table_tuple->push(&cols, sizeof cols);
        
        while(row_table = mysql_fetch_row(res))
        {
            for(int i = 0; i < cols; i++)
            {
                int data = 0;
                memcpy(&data, row_table[i], 4);
                table_tuple->push(&data, 4);
            }
        }
        table_tuple->sealing();
        
        // tuple에 합침
        tuple->push(table_tuple->getdata(), table_tuple->getLen() + 4);
        
        // table tupe 해체
        delete[] table_tuple->getdata();
        delete table_tuple;
    }
    
    tuple->sealing();
    
    send(sockfd, tuple, tuple->getLen() + 4, 0);
    
    delete[] tuple->getdata();
    delete tuple;
}

void STATEMANAGER::installDB ( TUPLE* tuple )
{
    if( !tuple->validity() )
    {
        printf("tuple is not valid, cannot install in %s\n", __func__);
        exit(0);
    }
    
    char* p = tuple->getcontent();
    ushort size = 0;
    
    if( tuple->getLen() != 0 )
    {
        while( size < tuple->getLen() )
        {
            // table name size?
            unsigned char nsize = 0;
            memcpy(&nsize, p + 4, 1);
            
            // table name?
            char* name = new char[nsize+1];
            memset(name, 0, nsize + 1);
            memcpy(&name, p + 5, nsize);
            
            p = p + 5 + nsize;
            
            // rows?
            unsigned short rows = 0;
            memcpy(&rows, p, 2);
            
            // cols?
            unsigned short cols = 0;
            memcpy(&cols, p + 2, 2);
            
            p += 4;
            
            for(int i = 0; i < rows; i++)
            {
                int uuid = 0;
                memcpy(&uuid, p, 4);
                p += 4;
                for(int j = 1; j < cols; j++)
                {
                    float value = 0;
                    memcpy(&value, p, 4);
                    p += 4;
                    
                    struct DBCONN* conn = NULL;
                    for(auto iter = conns.begin(); iter != conns.end(); ++iter)
                    {
                        struct DBCONN* conn_temp = *iter;
                        if(conn_temp->uuid == uuid)
                        {
                            conn = conn_temp;
                            break;
                        }
                    }
                    if(conn == NULL)
                    {
                        printf("tuple partial migration error in %s\n", __func__);
                        exit(0);
                    }
                    this->writeTable(conn, name, uuid, value, j);
                }
            }
            
            size += 4 + 1 + nsize + 2 + 2 + rows*cols*4;
        }
    }
}