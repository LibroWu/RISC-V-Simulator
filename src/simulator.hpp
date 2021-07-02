#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <iostream>
#include <cstring>
#include "Operator.h"
#include "binaryManager.h"

const int QUEUE_SIZE = 32;
const int mem_size = 4194304;
class simulator{
private:
    //private function
    unsigned char hexToDec(char c){
        return ((c<='9'&&c>='0')?c-'0':c-'A'+10);
    }

    unsigned int combineChars(int pos=-1,unsigned char len=4){
        unsigned int res=0;
        if (pos==-1) pos=pc;
        for (int i = pos+len-1; i >= pos; --i) {
            res=(res<<8)+memory[i];
        }
        return res;
    }
private:
    //private struct&class define
    struct regFile{
        unsigned int reg[32],Q[32];

        regFile(){
            memset(reg,0,sizeof(reg));
            memset(Q,0, sizeof(Q));
        }

        unsigned int& operator[](int pos) {
            return reg[pos];
        }

        unsigned int& operator()(int pos) {
            return Q[pos];
        }
    };

    template<class T,unsigned int len=QUEUE_SIZE>
    class loopQueue{
    private:
        int head,tail;
        //1 for full,-1 for empty, others 0
        char status;
        T que[len];
    public:
        loopQueue():head(0),tail(0),status(-1){}

        char getStatus(){
            return status;
        }

        int push(const T& t){
            que[tail++]=t;
            if (tail==len) tail=0;
            if (status==-1) status=0;
            if (tail==head) status=1;
            return tail-1;
        }

        void pop(){
            if (++head==len) head=0;
            if (status==1) status=0;
            if (head==tail) status=-1;
        }

        T& getFront(){
            return que[head];
        }

        T& operator[](int pos) {
            return que[pos];
        }
    };

    struct ROB_Node{
        unsigned int value;
        int linkToRs,linkToReg;
        bool  hasValue;
        ROB_Node():hasValue(0){}
    };

    class ROB{
    private:
        loopQueue<ROB_Node> robQue;
    public:
        bool isFull(){}
        void update(){}
        int reserve(){}
    };

    struct RS_Node{
        bool busy;
        int id,Q1,Q2,next;
        unsigned int V1,V2;
        baseOperator* opPtr;

        RS_Node():busy(0),opPtr(nullptr){}
    };

    struct SLBufferNode{

    };

    struct SLBuffer{

        void update(){}
    };

    struct RS{
        unsigned int _;
        int head;
        RS_Node rsQue[QUEUE_SIZE];
        char RS_status;
        RS():head(0){
            for (int i = 0; i < QUEUE_SIZE; ++i) {
                rsQue[i].next=i+1;
            }
        }
        //scan the whole station and return the command which is ready for execution
        int scan(){}

        void update(){}
    };

    //result buffer
    struct IssueResult{
        bool hasResult;
        baseOperator* basePtr;
        unsigned int Q1,Q2,V1,V2,id;
    };

    struct ExResult{

    };

    struct CommitResult{

    };




private:
    //private variable
    unsigned int pc;
    unsigned char* memory;
    regFile regPre,regNext;
    loopQueue<unsigned int> instFetchQue;
    //switches
    bool RS_is_stall;

    IssueResult issueResult;
    ROB rob;
    RS rs;
public:
	simulator(){
        memory=new unsigned char [mem_size];
        memset(memory,0, sizeof(memory));
	}
	void scan(){
        std::string s;
        unsigned int pos=0;
        while (std::cin>>s) {
            if (s=="#") break;
            if (s[0]=='@') {
                pos=0;
                for (int i = 1; i <= 8; ++i)
                    pos=(pos<<4)+ hexToDec(s[i]);
            } else {
                memory[pos++]= (hexToDec(s[0])<<4)+ hexToDec(s[1]);
            }
        }
	}
	unsigned int cycle = 0;
	void run(){
        while(true){
            /*在这里使用了两阶段的循环部分：
              1. 实现时序电路部分，即在每个周期初同步更新的信息。
              2. 实现逻辑电路部分，即在每个周期中如ex、issue的部分
              已在下面给出代码
            */
            run_rob();
            if(code_from_rob_to_commit == 0x0ff00513){
                std::cout << std::dec << ((unsigned int)regPre[10] & 255u);
                break;
            }
            run_slbuffer();
            run_reservation();
            run_inst_fetch_queue();
            update();

            run_ex();
            run_issue();
            run_commit();
        }
	}

    void run_inst_fetch_queue(){
        /*
        在这一部分你需要完成的工作：
        1. 实现一个先进先出的指令队列
        2. 读取指令并存放到指令队列中
        3. 准备好下一条issue的指令
        tips: 考虑边界问题（满/空...）
        */
        //todo:branch predict
        if (instFetchQue.getStatus()!=1) {
            instFetchQue.push(combineChars(pc));
            pc += 4;
        }
    }

    void run_issue(){
        /*
        在这一部分你需要完成的工作：
        1. 从run_inst_fetch_queue()中得到issue的指令
        2. 对于issue的所有类型的指令向ROB申请一个位置（或者也可以通过ROB预留位置），并修改regfile中相应的值
        2. 对于 非 Load/Store的指令，将指令进行分解后发到Reservation Station
          tip: 1. 这里需要考虑怎么得到rs1、rs2的值，并考虑如当前rs1、rs2未被计算出的情况，参考书上内容进行处理
               2. 在本次作业中，我们认为相应寄存器的值已在ROB中存储但尚未commit的情况是可以直接获得的，即你需要实现这个功能
                  而对于rs1、rs2不ready的情况，只需要stall即可，有兴趣的同学可以考虑下怎么样直接从EX完的结果更快的得到计算结果
        3. 对于 Load/Store指令，将指令分解后发到SLBuffer(需注意SLBUFFER也该是个先进先出的队列实现)
        tips: 考虑边界问题（是否还有足够的空间存放下一条指令）
        */
        if (instFetchQue.getStatus()!=0 && rs.head!=QUEUE_SIZE) {
            //set the command
            binaryManager command;
            command.setValue(instFetchQue.getFront());
            instFetchQue.pop();
            //ID
            unsigned int immediate, npc = pc;
            unsigned char opcode = command.slice(0, 6), func3 = 0, func7 = 0;
            char rd = -1, rs1 = -1, rs2 = -1;
            baseOperator *basePtr;
            OpType Type;
            switch (opcode) {
                case 55:
                case 23:
                    //LUI U-type
                    //AUIPC U-type
                    immediate = command.slice(12, 31) << 12;
                    basePtr = new UtypeOperator;
                    Type=U;
                    break;
                case 111:
                    //JAL J-type
                    immediate = (command[31] * ((1 << 12) - 1) << 20) + (command.slice(12, 19) << 12) +
                                (command[20] << 11) + (command.slice(25, 30) << 5) + (command.slice(21, 24) << 1);
                    basePtr = new JtypeOperator;
                    Type=J;
                    break;
                case 99:
                    //B-type
                    immediate =
                            (command[31] * ((1 << 20) - 1) << 12) + (command[7] << 11) + (command.slice(25, 30) << 5) +
                            (command.slice(8, 11) << 1);
                    func3 = command.slice(12, 14);
                    rs1 = command.slice(15, 19);
                    rs2 = command.slice(20, 24);
                    basePtr = new BtypeOperator;
                    Type=B;
                    break;
                case 3:
                case 19:
                case 103:
                    //I-type
                    //JALR I-type
                    func3 = command.slice(12, 14);
                    immediate = (command[31] * ((1 << 21) - 1) << 11) + command.slice(20, 30);
                    rd = command.slice(7, 11);
                    rs1 = command.slice(15, 19);
                    basePtr = new ItypeOperator;
                    Type=I;
                    break;
                case 35:
                    //S-type
                    immediate = (command[31] * ((1 << 21) - 1) << 11) + (command.slice(25, 30) << 5) +
                                (command.slice(8, 11) << 1) + command[7];
                    func3 = command.slice(12, 14);
                    rs1 = command.slice(15, 19);
                    basePtr = new StypeOperator;
                    Type=S;
                    break;
                case 51:
                    //R-type
                    rd = command.slice(7, 11);
                    rs1 = command.slice(15, 19);
                    rs2 = command.slice(20, 24);
                    func3 = command.slice(12, 14);
                    func7 = command.slice(25, 31);
                    basePtr = new RtypeOperator;
                    Type=R;
                    break;
            }
            basePtr->setValue(opcode, func3, func7, immediate, npc,Type);
            issueResult.hasResult= true;
            issueResult.basePtr=basePtr;
            if (rd!=-1) {
                regNext(rd);
            }
            if (rs1!=-1) {

            } else {

            }
            if (rs2!=-1) {

            } else {

            }
        } else {
            issueResult.hasResult= false;
        }
    }

    void run_reservation(){
        /*
        在这一部分你需要完成的工作：
        1. 设计一个Reservation Station，其中要存储的东西可以参考CAAQA或其余资料，至少需要有用到的寄存器信息等
        2. 如存在，从issue阶段收到要存储的信息，存进Reservation Station（如可以计算也可以直接进入计算）
        3. 从Reservation Station或者issue进来的指令中选择一条可计算的发送给EX进行计算
        4. 根据上个周期EX阶段或者SLBUFFER的计算得到的结果遍历Reservation Station，更新相应的值
        */
    }

    void run_ex(){
        /*
        在这一部分你需要完成的工作：
        根据Reservation Station发出的信息进行相应的计算
        tips: 考虑如何处理跳转指令并存储相关信息
              Store/Load的指令并不在这一部分进行处理
        */
    }

    void run_slbuffer(){
        /*
        在这一部分中，由于SLBUFFER的设计较为多样，在这里给出两种助教的设计方案：
        1. 1）通过循环队列，设计一个先进先出的SLBUFFER，同时存储 head1、head2、tail三个变量。
           其中，head1是真正的队首，记录第一条未执行的内存操作的指令；tail是队尾，记录当前最后一条未执行的内存操作的指令。
           而head2负责确定处在head1位置的指令是否可以进行内存操作，其具体实现为在ROB中增加一个head_ensure的变量，每个周期head_ensure做取模意义下的加法，直到等于tail或遇到第一条跳转指令，
           这个时候我们就可以保证位于head_ensure及之前位置的指令，因中间没有跳转指令，一定会执行。因而，只要当head_ensure当前位置的指令是Store、Load指令，我们就可以向slbuffer发信息，增加head2。
           简单概括即对head2之前的Store/Load指令，我们根据判断出ROB中该条指令之前没有jump指令尚未执行，从而确定该条指令会被执行。

           2）同时SLBUFFER还需根据上个周期EX和SLBUFFER的计算结果遍历SLBUFFER进行数据的更新。

           3）此外，在我们的设计中，将SLBUFFER进行内存访问时计算需要访问的地址和对应的读取/存储内存的操作在SLBUFFER中一并实现，
           也可以考虑分成两个模块，该部分的实现只需判断队首的指令是否能执行并根据指令相应执行即可。

        2. 1）SLB每个周期会查看队头，若队头指令还未ready，则阻塞。
           
           2）当队头ready且是load指令时，SLB会直接执行load指令，包括计算地址和读内存，
           然后把结果通知ROB，同时将队头弹出。ROB commit到这条指令时通知Regfile写寄存器。
           
           3）当队头ready且是store指令时，SLB会等待ROB的commit，commit之后会SLB执行这
           条store指令，包括计算地址和写内存，写完后将队头弹出。
        */
        //todo: method 2
    }

    void run_rob(){
        /*
        在这一部分你需要完成的工作：
        1. 实现一个先进先出的ROB，存储所有指令
        1. 根据issue阶段发射的指令信息分配空间进行存储。
        2. 根据EX阶段和SLBUFFER的计算得到的结果，遍历ROB，更新ROB中的值
        3. 对于队首的指令，如果已经完成计算及更新，进行commit
        */
    }

    void run_commit(){
        /*
        在这一部分你需要完成的工作：
        1. 根据ROB发出的信息更新寄存器的值，包括对应的ROB和是否被占用状态（注意考虑issue和commit同一个寄存器的情况）
        2. 遇到跳转指令更新pc值，并发出信号清空所有部分的信息存储（这条对于很多部分都有影响，需要慎重考虑）
        */
    }

    void update(){
        /*
        在这一部分你需要完成的工作：
        对于模拟中未完成同步的变量（即同时需记下两个周期的新/旧信息的变量）,进行数据更新。
        */
        regPre=regNext;
    }
	~simulator(){delete [] memory;} 
};

#endif