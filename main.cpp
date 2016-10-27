#include <stdio.h>
#include "ExecutorManager.h"

using namespace std;

int mygetch(void)
{
    struct termios oldt,
    newt;
    int ch;
    tcgetattr( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    return ch;
}

int main(int argc, char **argv) 
{
    //Task* task;
    ExecutorManager executormanager;
    
    // 익스큐터 생성 후, 내부에서 인자를 이용하여 커넥션을 생성
    executormanager.createExecutor(1237);
    
    // 등록된 모든 익스큐터 런런런
    executormanager.executorRunAll();
    
    // Mainthread는 컨트롤 명령을 받도록 프로그래밍
    mygetch();
    
    return 0;
}
