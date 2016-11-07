
class PACKET
{
private:
    char* bytearray;
    unsigned int size;
    unsigned char seq;
public:
    PACKET(char* in_array, unsigned int size, unsigned char seq);
    
    char* getArray();
    unsigned int getSize();
    unsigned char getSeq();
    void setArray(char* arr);
    void setSize(unsigned int size);
};