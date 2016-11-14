#include <mysql/mysql.h>
#include <list>

// createdb(key_name, tuple_window)
// writedb(key_value, value)
// readdb(key_value)

using namespace std;

class TUPLE;

struct DBCONN
{
    MYSQL connection;
    unsigned int uuid;
};

class STATEMANAGER
{
private:
    char user[20];
    char password[30];
    char db_name[20];
    int port;
    list<DBCONN*> conns;
public:
    STATEMANAGER(char* uesr, char* password, char* db_name);
    struct DBCONN* getconn(unsigned int uuid);
    void writeTable(struct DBCONN* db, char* table_name, int key_value, double value, int column = 0);
    TUPLE* readTable(DBCONN* db, char* table_name, int key_value);
    void migrateDB(int sockfd, unsigned int uuid);
    void installDB(TUPLE* tuple);
};