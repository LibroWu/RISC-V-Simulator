#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <iostream>
#include <fstream>
#include <cstring>
#include <utility>
#include <iomanip>
#include <vector>
#include "Operator.h"
#include "binaryManager.h"

const int QUEUE_SIZE = 32;
const int mem_size = 4194304;
#define debugs

#ifdef debugs
using std::fstream;
using std::cout;
fstream debug_out("debug.out", std::ios::out);
#endif

class simulator {
public:
    //private function
    unsigned char hexToDec(char c) { return ((c <= '9' && c >= '0') ? c - '0' : c - 'A' + 10); }

    unsigned int combineChars(unsigned int pos, unsigned char len = 4) {
        unsigned int res = 0;
        for (int i = pos + len - 1; i >= int(pos); --i) {
            res = (res << 8) + memory[i];
        }
        return res;
    }

public:
    //private struct&class define
    struct regFile {
        unsigned int reg[32];
        int Q[32];

        regFile() {
            memset(reg, 0, sizeof(reg));
            memset(Q, 0, sizeof(Q));
        }

        unsigned int &operator[](int pos) { return reg[pos]; }

        int &operator()(int pos) { return Q[pos]; }

        void clearQ() { memset(Q, 0, sizeof(Q)); }
    };

    struct SLBuffer;

    template<class T, unsigned int len = QUEUE_SIZE>
    class loopQueue {
        friend SLBuffer;
    private:
        int head, tail;
        //1 for full,-1 for isEmpty, others 0
        char status;
        T que[len];
    public:
        int size;

        int reserve() {
            ++size;
            int preTail = tail;
            tail++;
            if (tail == len) tail = 0;
            if (status == -1) status = 0;
            if (tail == head) status = 1;
            return preTail + 1;
        }

        int getTail() { return tail + 1; }

        loopQueue() : head(0), tail(0), status(-1), size(0) {
            memset(que, 0, sizeof(que));
        }

        char getStatus() { return status; }

        int push(const T &t) {
            ++size;
            int prePail = tail;
            que[tail++] = t;
            if (tail == len) tail = 0;
            if (status == -1) status = 0;
            if (tail == head) status = 1;
            return prePail + 1;
        }

        void pop() {
            --size;
            if (++head == len) head = 0;
            if (status == 1) status = 0;
            if (head == tail) status = -1;
        }

        T &getFront() { return que[head]; }

        T &operator[](int pos) { return que[pos - 1]; }

        void clear() {
            memset(que, 0, sizeof(que));
            head = tail = 0;
            size = 0;
            status = -1;
        }
    };

    struct ROB_Node {
        unsigned int value, predict_pc, npc, origin_code;
        int linkToReg, id;
        bool hasValue, isJump, isStore;

        ROB_Node() : hasValue(0), isJump(0) {}
    };

    class ROB {
    private:
        loopQueue<ROB_Node> preQue, nextQue;
    public:
        bool isFull() { return nextQue.getStatus() == 1; }

        void update() { preQue = nextQue; }

        int getTail() { return preQue.getTail(); }

        int reserve() {
            int pos = nextQue.reserve();
            nextQue[pos].id = pos;
            return pos;
        }

        ROB_Node &getFront() { return preQue.getFront(); }

        ROB_Node &operator[](int pos) { return preQue[pos]; }

        ROB_Node &operator()(int pos) { return nextQue[pos]; }

        void pop() { nextQue.pop(); }

        void clear() {
            preQue.clear();
            nextQue.clear();
        }

        bool isEmpty() {
            return preQue.getStatus() == -1;
        }

        int getSize() {
            return preQue.size;
        }
    };

    struct RS_Node {
        bool busy;
        int id, Q1, Q2, next, origin_code;
        unsigned int V1, V2;
        baseOperator *opPtr;

        RS_Node() : busy(0), opPtr(nullptr) {}

        void setValue(bool Busy, int Id, int QI, int QII, unsigned int VI, unsigned int VII, baseOperator *OpPtr) {
            busy = Busy, id = Id, Q1 = QI, Q2 = QII, V1 = VI, V2 = VII, opPtr = OpPtr;
        }

        bool canEx() {
            return (Q1 == 0 && Q2 == 0);
        }

        bool match(int pos) {
            return (Q1 == pos || Q2 == pos);
        }

        void setValue(int pos, unsigned int v) {
            if (Q1 == pos) {
                V1 = v;
                Q1 = 0;
            }
            if (Q2 == pos) {
                V2 = v;
                Q2 = 0;
            }
        }

        void clear() {
            busy = 0;
            Q1 = Q2 = V1 = V2 = next = 0;
            opPtr = nullptr;
        }
    };

    struct RS {
        struct ResultBuffer {
            int head;
            RS_Node rsQue[QUEUE_SIZE];

            ResultBuffer() : head(0) {
                for (int i = 0; i < QUEUE_SIZE; ++i) rsQue[i].next = i + 1;
            }

            void clear() {
                head = 0;
                for (int i = 0; i < QUEUE_SIZE; ++i) {
                    rsQue[i].clear();
                    rsQue[i].next = i + 1;
                }
            }
        } preBuffer, nextBuffer;

        RS_Node exNode;
        bool exFlag;

        RS() : exFlag(0) {}

        //scan the whole station and return the command which is ready for execution
        bool scan() {
            for (int j = 0; j < QUEUE_SIZE; ++j)
                if (preBuffer.rsQue[j].busy && preBuffer.rsQue[j].canEx()) {
                    exNode = preBuffer.rsQue[j];
                    remove(j);
                    return true;
                }
            return false;
        }

#ifdef debugs

        void list() {
            for (int j = 0; j < QUEUE_SIZE; ++j)
                if (preBuffer.rsQue[j].busy) {
                    cout << std::dec << j << ' ' << preBuffer.rsQue[j].Q1 << ' ' << preBuffer.rsQue[j].V1 << ' '
                         << preBuffer.rsQue[j].Q2 << ' ' << preBuffer.rsQue[j].V2 << ' '
                         << preBuffer.rsQue[j].id << ' ' << preBuffer.rsQue[j].opPtr->opType << std::hex << std::endl;
                }
        }

#endif

        void update() { preBuffer = nextBuffer; }

        void insert(const RS_Node &rsNode) {
            int next = nextBuffer.rsQue[nextBuffer.head].next;
            nextBuffer.rsQue[nextBuffer.head] = rsNode;
            nextBuffer.head = next;
        }

        void remove(int pos) {
            nextBuffer.rsQue[pos].clear();
            nextBuffer.rsQue[pos].next = nextBuffer.head;
            nextBuffer.head = pos;
        }

        //get the value in pre
        RS_Node &operator[](int pos) { return preBuffer.rsQue[pos]; }

        //get the value in next
        RS_Node &operator()(int pos) { return nextBuffer.rsQue[pos]; }

        bool isFull() { return nextBuffer.head == QUEUE_SIZE; }

        void clear() {
            exFlag = false;
            preBuffer.clear();
            nextBuffer.clear();
        }
    };

    struct SLBufferNode {
        char exCount;

        RS_Node rsNode;
        bool hasCommit;

        SLBufferNode() : exCount(0), hasCommit(false) {}

        bool ready() {
            if (rsNode.opPtr->opType == S) return (rsNode.Q1 == 0 && rsNode.Q2 == 0 && hasCommit);
            return rsNode.Q1 == 0;
        }
    };

    struct SLBuffer {
        bool hasResult, isStore, newHasResult, newIsStore;
        unsigned int value, newValue;
        int posROB, newPosROB;

        SLBuffer() : hasResult(false), isStore(false), value(0), posROB(0),
                     newHasResult(false), newIsStore(false), newValue(0), newPosROB(0) {}

        loopQueue<SLBufferNode> preQue, nextQue;

        void update() {
            preQue = nextQue;
            value = newValue;
            hasResult = newHasResult;
            isStore = newIsStore;
            posROB = newPosROB;
        }

        bool isFull() { return nextQue.getStatus() == 1; }

        bool isEmpty() { return preQue.getStatus() == -1; }

        void push(const SLBufferNode &node) { nextQue.push(node); }

        void pop() { nextQue.pop(); }

        SLBufferNode &operator[](int pos) { return preQue[pos]; }

        SLBufferNode &operator()(int pos) { return nextQue[pos]; }

        void traverse_commit(int posROB) {
            int i = nextQue.head;
            bool flag = true;
            while (i != nextQue.tail || flag && nextQue.status == 1) {
                flag = false;
                if (nextQue[i + 1].rsNode.id == posROB) {
                    nextQue[i + 1].hasCommit = true;
                }
                ++i;
                if (i == QUEUE_SIZE) i = 0;
            }
        }

        void traverse(int posROB, unsigned int value) {
            int i = nextQue.head;
            bool flag = true;
            while (i != nextQue.tail || flag && nextQue.status == 1) {
                flag = false;
                if (nextQue[i + 1].rsNode.opPtr->opType == S) {
                    if (nextQue[i + 1].rsNode.Q1 == posROB) {
                        nextQue[i + 1].rsNode.Q1 = 0;
                        nextQue[i + 1].rsNode.V1 = value;
                    }
                    if (nextQue[i + 1].rsNode.Q2 == posROB) {
                        nextQue[i + 1].rsNode.Q2 = 0;
                        nextQue[i + 1].rsNode.V2 = value;
                    }
                } else {
                    if (nextQue[i + 1].rsNode.Q1 == posROB) {
                        nextQue[i + 1].rsNode.Q1 = 0;
                        nextQue[i + 1].rsNode.V1 = value;
                    }
                }
                ++i;
                if (i == QUEUE_SIZE) i = 0;
            }
        }

        void clear() {
            newHasResult = false;
            std::vector<SLBufferNode> vec_slbuffer;
            while (!(nextQue.getStatus() == -1) && nextQue.getFront().rsNode.opPtr->opType==S && nextQue.getFront().ready()) {
               vec_slbuffer.push_back(nextQue.getFront());
                nextQue.pop();
            }
            nextQue.clear();
            for (int i = 0; i < vec_slbuffer.size(); ++i) nextQue.push(vec_slbuffer[i]);
            /*
            bool flag = nextQue.getStatus() == -1;
            SLBufferNode tmp;
            if (!flag) tmp = nextQue.getFront();
            nextQue.clear();
            if (!flag && tmp.ready()) nextQue.push(tmp);*/
            preQue = nextQue;
        }
    };

    //result buffer
    struct IssueResult {
        bool hasResult, toRS, toSLBuffer;;
        unsigned code;
        RS_Node rsNode;

        IssueResult() : hasResult(0), toRS(0), toSLBuffer(0) {}
    };

    struct ExResult {
        bool hasResult;
        unsigned int value, npc;
        int posROB;

        ExResult() : hasResult(0), value(0), npc(0), posROB(0) {}
    };

    struct ChannelToRegfile {
        int issue_rd, issue_pos, commit_rd, id;
        unsigned int commit_value;

        ChannelToRegfile() : issue_rd(-1), issue_pos(-1), commit_rd(-1), commit_value(0) {}

        void clear() {
            issue_rd = issue_pos = commit_rd = -1;
            commit_value = 0;
        }
    };

    struct tuple {
        unsigned int first, second, third;
    };

private:
    //private variable
    unsigned int pc, next_pc, code_from_rob_to_commit, reserve_predict_pc, reserve_origin_code;
    unsigned char *memory;
    int reserve_link_to_reg, commit_to_SLB_id;
    regFile regPre, regNext;
    loopQueue<tuple> preFetchQue, nextFetchQue;
    //switches
    bool RS_is_stall, SLBuffer_is_stall, commit_flag, reserve_flag, reserve_isJump, reserve_isStore;
    bool fetch_flag, ROB_is_stall, commit_to_SLB;
    bool issue_to_ex_flag;
    RS_Node issue_to_ex_node;

    IssueResult issueResult;
    ExResult exResult;
    ROB rob;
    RS rs;
    SLBuffer slBuffer;
    ChannelToRegfile channelToRegfile;

    loopQueue<baseOperator *,1000> vec_basePtr;
public:
    simulator() : pc(0), next_pc(0), memory(new unsigned char[mem_size]) {
        RS_is_stall = ROB_is_stall = SLBuffer_is_stall = false;
        issue_to_ex_flag = commit_flag = commit_to_SLB = reserve_flag = reserve_isStore = reserve_isJump = fetch_flag = false;
        reserve_origin_code = 0;
        memset(memory, 0, sizeof(memory));
    }

    void scan() {
        std::string s;
        unsigned int pos = 0;
        while (std::cin >> s) {
            if (s == "#") break;
            if (s[0] == '@') {
                pos = 0;
                for (int i = 1; i <= 8; ++i)
                    pos = (pos << 4) + hexToDec(s[i]);
            } else {
                memory[pos++] = (hexToDec(s[0]) << 4) + hexToDec(s[1]);
            }
        }
    }

    unsigned int cycle = 0;
    unsigned int reg_count = 0;
    unsigned int mem_count = 0;

    void run() {
        while (true) {
            ++cycle;
            /*在这里使用了两阶段的循环部分：
              1. 实现时序电路部分，即在每个周期初同步更新的信息。
              2. 实现组合电路部分，即在每个周期中如ex、issue的部分
              已在下面给出代码
            */
            run_rob();
            if (code_from_rob_to_commit == 0x0ff00513) {
                std::cout << std::dec << ((unsigned int) regPre[10] & 255u)<<std::endl;
                //std::cout << std::dec << ((unsigned int) cycle)<<std::endl;
                break;
            }
            run_slbuffer();
            run_reservation();
            run_regfile();
            run_inst_fetch_queue();
            update();

            run_ex();
            run_issue();
            run_commit();
        }
    }

    void run_inst_fetch_queue() {
        /*
        在这一部分你需要完成的工作：
        1. 实现一个先进先出的指令队列
        2. 读取指令并存放到指令队列中
        3. 准备好下一条issue的指令
        tips: 考虑边界问题（满/空...）
        */
        //todo:branch predict
        binaryManager command;
        pc = next_pc;
        if (fetch_flag) nextFetchQue.pop();
        if (nextFetchQue.getStatus() != 1) {
            command.setValue(combineChars(pc));
            if (command.slice(0, 6) == 111) {
                //JAL J-type
                next_pc = pc + (command[31] * ((1 << 12) - 1) << 20) + (command.slice(12, 19) << 12) +
                          (command[20] << 11) + (command.slice(25, 30) << 5) + (command.slice(21, 24) << 1);
            } else next_pc = pc + 4;
            nextFetchQue.push({(unsigned int) command, next_pc, pc});
        }
    }

    void run_issue() {
        /*
        在这一部分你需要完成的工作：
        1. 从run_inst_fetch_queue()中得到issue的指令
        2. 对于issue的所有类型的指令向ROB申请一个位置（或者也可以通过ROB预留位置），并通知regfile修改相应的值
        2. 对于 非 Load/Store的指令，将指令进行分解后发到Reservation Station
          tip: 1. 这里需要考虑怎么得到rs1、rs2的值，并考虑如当前rs1、rs2未被计算出的情况，参考书上内容进行处理
               2. 在本次作业中，我们认为相应寄存器的值已在ROB中存储但尚未commit的情况是可以直接获得的，即你需要实现这个功能
                  而对于rs1、rs2不ready的情况，只需要stall即可，有兴趣的同学可以考虑下怎么样直接从EX完的结果更快的得到计算结果
        3. 对于 Load/Store指令，将指令分解后发到SLBuffer(需注意SLBUFFER也该是个先进先出的队列实现)
        tips: 考虑边界问题（是否还有足够的空间存放下一条指令）
        */
        reserve_flag = false;
        issueResult.hasResult = false;
        fetch_flag = false;
        if (preFetchQue.getStatus() != -1 && !ROB_is_stall) {
            //set the command
            binaryManager command;
            command.setValue(preFetchQue.getFront().first);
            issueResult.code = command;
         //   std::cout << "(" << std::hex << preFetchQue.getFront().third << ' ' << (unsigned int) command<<")"<<std::dec<<std::endl;
/*            if (preFetchQue.getFront().third == 4164)
                cout << "waeaweaw\n";
            std::cout << std::hex << preFetchQue.getFront().third << ' ' << (unsigned int) command << ' ' << regPre[1] << ' ' << regPre[2] << ' '
                      << regPre[10] << ' ' << regPre[15] << ' ' << regPre[14] << std::endl;
            if (!rob.isEmpty())
                std::cout << "ROB: " << rob.getFront().origin_code << ' ' << rob.getFront().value << ' '
                          << rob.getFront().hasValue << ' ' << rob.getFront().id << std::endl;
            std::cout << rob.getSize() << ' ' << rob.isFull() << std::endl;
            std::cout << "RS List: " << std::endl;
            rs.list();
            if (slBuffer.preQue.getStatus() != -1)
                std::cout << "SLB: " << int(slBuffer.preQue.getFront().exCount) << ' '
                          << slBuffer.preQue.getFront().ready() << ' ' << slBuffer.preQue.getFront().rsNode.Q1 << ' '
                          << slBuffer.preQue.getFront().rsNode.Q2 << ' ' << slBuffer.preQue.getFront().hasCommit << ' '
                          << slBuffer.preQue.getFront().rsNode.id << ' ' << slBuffer.preQue.getFront().rsNode.V2 << ' '
                          << slBuffer.preQue.getFront().rsNode.origin_code << std::endl;
            std::cout << "---------------------------------\n";*/
            fetch_flag = true;
            //ID
            bool illegal_command = false;
            int Q1, Q2;
            unsigned int immediate, npc = preFetchQue.getFront().third, V1, V2;
            unsigned char opcode = command.slice(0, 6), func3 = 0, func7 = 0;
            char rd = -1, rs1 = -1, rs2 = -1;
            issueResult.toRS = issueResult.toSLBuffer = false;
            baseOperator *basePtr;
            OpType Type;
            switch (opcode) {
                case 55:
                case 23:
                    //LUI U-type
                    //AUIPC U-type
                    immediate = command.slice(12, 31) << 12;
                    basePtr = new UtypeOperator;
                    rd = command.slice(7, 11);
                    Type = U;
                    issueResult.toRS = true;
                    break;
                case 111:
                    //JAL J-type
                    immediate = (command[31] * ((1 << 12) - 1) << 20) + (command.slice(12, 19) << 12) +
                                (command[20] << 11) + (command.slice(25, 30) << 5) + (command.slice(21, 24) << 1);
                    basePtr = new JtypeOperator;
                    rd = command.slice(7, 11);
                    Type = J;
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
                    Type = B;
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
                    Type = I;
                    break;
                case 35:
                    //S-type
                    immediate = (command[31] * ((1 << 21) - 1) << 11) + (command.slice(25, 30) << 5) +
                                (command.slice(8, 11) << 1) + command[7];
                    func3 = command.slice(12, 14);
                    rs1 = command.slice(15, 19);
                    rs2 = command.slice(20, 24);
                    basePtr = new StypeOperator;
                    Type = S;
                    break;
                case 51:
                    //R-type
                    rd = command.slice(7, 11);
                    rs1 = command.slice(15, 19);
                    rs2 = command.slice(20, 24);
                    func3 = command.slice(12, 14);
                    func7 = command.slice(25, 31);
                    basePtr = new RtypeOperator;
                    Type = R;
                    break;
                default:
                    illegal_command = true;
            }
            if (!illegal_command) {
                if (vec_basePtr.getStatus()==1) {
                    delete vec_basePtr.getFront();
                    vec_basePtr.pop();
                }
                vec_basePtr.push(basePtr);
                basePtr->setValue(opcode, func3, func7, immediate, npc, Type);
                int posROB = rob.getTail();
                if (rd != -1 && rd != 0) {
                    channelToRegfile.issue_rd = rd;
                    channelToRegfile.issue_pos = posROB;
                } else {
                    channelToRegfile.issue_rd = -1;
                }
                if (rs1 != -1) {
                    if ((Q1 = regPre(rs1)) == 0) V1 = regPre[rs1];
                    else if (rob[Q1].hasValue) {
                        V1 = rob[Q1].value;
                        Q1 = 0;
                    }/* else if (exResult.posROB == Q1) {
                        Q1 = 0;
                        V1 = exResult.value;
                    }*/
                } else Q1 = 0;
                if (rs2 != -1) {
                    if ((Q2 = regPre(rs2)) == 0) V2 = regPre[rs2];
                    else if (rob[Q2].hasValue) {
                        V2 = rob[Q2].value;
                        Q2 = 0;
                    } /*else if (exResult.posROB == Q2) {
                        Q2 = 0;
                        V2 = exResult.value;
                    }*/
                } else Q2 = 0;
                issueResult.rsNode.setValue(true, posROB, Q1, Q2, V1, V2, basePtr);
                if (opcode == 35 || opcode == 3) issueResult.toSLBuffer = true;
                else issueResult.toRS = true;
                //ID end
                //set the signal
                issueResult.hasResult = true;
                reserve_flag = true;
                reserve_isStore = issueResult.rsNode.opPtr->opType == S;
                reserve_origin_code = (unsigned int) (command);
                reserve_link_to_reg = (rd == 0) ? -1 : rd;
                reserve_isJump = (opcode == 99 || opcode == 103);
                reserve_predict_pc = preFetchQue.getFront().second;
                issue_to_ex_flag = false;
                issueResult.rsNode.origin_code=reserve_origin_code;
                if (issueResult.toRS && issueResult.rsNode.canEx()) {
                    //send to ex directly
                    issueResult.hasResult = false;
                    issue_to_ex_flag = true;
                    issue_to_ex_node = issueResult.rsNode;
                }
                //may not happen
                if (issueResult.toRS && RS_is_stall || issueResult.toSLBuffer && SLBuffer_is_stall) {
                    issueResult.hasResult = false;
                    reserve_flag = false;
                    fetch_flag = false;
                }
                return;
            }
        }
    }

    void run_reservation() {
        /*
        在这一部分你需要完成的工作：
        1. 设计一个Reservation Station，其中要存储的东西可以参考CAAQA或其余资料，至少需要有用到的寄存器信息等
        2. 如存在，从issue阶段收到要存储的信息，存进Reservation Station（如可以计算也可以直接进入计算）
        3. 从Reservation Station或者issue进来的指令中选择一条可计算的发送给EX进行计算
        4. 根据上个周期EX阶段或者SLBUFFER的计算得到的结果遍历Reservation Station，更新相应的值
        */
        if (issueResult.hasResult && issueResult.toRS) {
            rs.insert(issueResult.rsNode);
        }
        rs.exFlag = rs.scan();
        if (rs.exFlag && issue_to_ex_flag) {
            rs.insert(issue_to_ex_node);
            issue_to_ex_flag= false;
        }
        //todo update with the result of ex or slbuffer
        if (exResult.hasResult) {
            for (int i = 0; i < QUEUE_SIZE; ++i)
                if (rs(i).busy && rs(i).match(exResult.posROB))
                    rs(i).setValue(exResult.posROB, exResult.value);
        }
        if (slBuffer.hasResult) {
            for (int i = 0; i < QUEUE_SIZE; ++i)
                if (rs(i).busy && rs(i).match(slBuffer.posROB))
                    rs(i).setValue(slBuffer.posROB, slBuffer.value);
        }
        RS_is_stall = rs.isFull();
    }

    void run_ex() {
        /*
        在这一部分你需要完成的工作：
        根据Reservation Station发出的信息进行相应的计算
        tips: 考虑如何处理跳转指令并存储相关信息
              Store/Load的指令并不在这一部分进行处理
        */
        exResult.hasResult = true;
        if (issue_to_ex_flag) {
            RS_Node &tmp = issue_to_ex_node;
            exResult.posROB = tmp.id;
            tmp.opPtr->operate(exResult.value, tmp.V1, tmp.V2);
            exResult.npc = tmp.opPtr->getNpc();
            issue_to_ex_flag = false;
        } else if (rs.exFlag) {
            RS_Node &tmp = rs.exNode;
            exResult.posROB = tmp.id;
            tmp.opPtr->operate(exResult.value, tmp.V1, tmp.V2);
            exResult.npc = tmp.opPtr->getNpc();
            rs.exFlag = false;
        } else {
            exResult.hasResult = false;
        }
    }

    void run_slbuffer() {
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

           4）同时SLBUFFER还需根据上个周期EX和SLBUFFER的计算结果遍历SLBUFFER进行数据的更新。
        */

        slBuffer.newHasResult = false;
        if (issueResult.hasResult && issueResult.toSLBuffer) {
            SLBufferNode tmp;
            tmp.rsNode = issueResult.rsNode;
            if (slBuffer.isEmpty()) {
                if (tmp.ready()) ++tmp.exCount;
            }
            slBuffer.push(tmp);
        }
        if (!slBuffer.isEmpty()) {
            SLBufferNode &front = slBuffer.preQue.getFront();
            if (front.ready()) {
                ++front.exCount;
                if (front.exCount == 3) {
                    unsigned int st;
                    front.rsNode.opPtr->operate(st, front.rsNode.V1, front.rsNode.V2);
                    if (front.rsNode.opPtr->opType == S) {
                       // std::cerr<<std::hex<<front.rsNode.origin_code<<std::dec<<std::endl;
                        //std::cout<<front.rsNode.origin_code<<std::endl;
                        switch (front.rsNode.opPtr->getFunc3()) {
                            case 0:
                                //SB
                                memory[st] = (front.rsNode.V2 & 0b11111111);
                                ++mem_count;/*
                                if (mem_count==251)
                                    std::cout<<"Waeaweawe\n";*/
                                //std::cerr<<"mem["<<st<<"]"<<int(memory[st])<<std::endl;
                                break;
                            case 1:
                                //SH
                                for (int i = st; i < st + 2; ++i) {
                                    memory[i] = front.rsNode.V2 & 0b11111111;
                                    ++mem_count;/*
                                    if (mem_count==251)
                                        std::cout<<"Waeaweawe\n";*/
                                    //std::cerr<<"mem["<<i<<"]"<<int(memory[i])<<std::endl;
                                    front.rsNode.V2 >>= 8;
                                }
                                break;
                            case 2:
                                //SW
                                for (int i = st; i < st + 4; ++i) {
                                    ++mem_count;/*
                                    if (mem_count==251)
                                        std::cout<<"Waeaweawe\n";*/
                                    memory[i] = front.rsNode.V2 & 0b11111111;
                                   // std::cerr<<"mem["<<i<<"]"<<int(memory[i])<<std::endl;
                                    front.rsNode.V2 >>= 8;
                                }
                                break;
                        }
                        slBuffer.newIsStore = true;
                        slBuffer.newHasResult = false;
                    } else {
                        unsigned char t;
                        switch (front.rsNode.opPtr->getFunc3()) {
                            case 0:
                                //LB
                                t = memory[st];
                                front.rsNode.V2 = ((t & (1 << 7)) * ((1 << 24) - 1) << 1) + t;
                                break;
                            case 1:
                                //LH
                                t = combineChars(st, 2);
                                front.rsNode.V2 = ((t & (1 << 15)) * ((1 << 16) - 1) << 1) + t;
                                break;
                            case 2:
                                //LW
                                front.rsNode.V2 = combineChars(st, 4);
                                break;
                            case 4:
                                //LBU
                                front.rsNode.V2 = memory[st];
                                break;
                            case 5:
                                //LHU
                                front.rsNode.V2 = combineChars(st, 2);
                                break;
                        }
                        slBuffer.newValue = front.rsNode.V2;
                        slBuffer.newIsStore = false;
                        slBuffer.newHasResult = true;
                    }

                    slBuffer.newPosROB = front.rsNode.id;
                    slBuffer.pop();
                } else slBuffer.nextQue.getFront() = front;
            }
        }
        if (commit_to_SLB) //slBuffer.nextQue.getFront().hasCommit = true;
            slBuffer.traverse_commit(commit_to_SLB_id);
        if (exResult.hasResult) {
            slBuffer.traverse(exResult.posROB, exResult.value);
        }
        if (slBuffer.hasResult && !slBuffer.isStore)  {
            slBuffer.traverse(slBuffer.posROB, slBuffer.value);
        }
        SLBuffer_is_stall = slBuffer.isFull();
    }

    void run_rob() {
        /*
        在这一部分你需要完成的工作：
        1. 实现一个先进先出的ROB，存储所有指令
        1. 根据issue阶段发射的指令信息分配空间进行存储。
        2. 根据EX阶段和SLBUFFER的计算得到的结果，遍历ROB，更新ROB中的值
        3. 对于队首的指令，如果已经完成计算及更新，进行commit
        */
        if (reserve_flag) {
            int pos = rob.reserve();
            rob(pos).linkToReg = reserve_link_to_reg;
            rob(pos).isStore = reserve_isStore;
            if (reserve_isStore) rob(pos).hasValue = true;
            else rob(pos).hasValue = false;
            rob(pos).isJump = reserve_isJump;
            rob(pos).predict_pc = reserve_predict_pc;
            rob(pos).origin_code = reserve_origin_code;
            reserve_flag = false;
        }
        if (commit_flag) {
            rob.pop();
            commit_flag = false;
        }
        if (exResult.hasResult) {
            ROB_Node &tmp = rob(exResult.posROB);
            tmp.hasValue = true;
            tmp.value = exResult.value;
            tmp.npc = exResult.npc;
        }
        if (slBuffer.hasResult && !slBuffer.isStore) {
            ROB_Node &tmp = rob(slBuffer.posROB);
            tmp.hasValue = true;
            tmp.value = slBuffer.value;
        }
        ROB_is_stall = rob.isFull();
    }

    void run_regfile() {
        /*
        每个寄存器会记录Q和V，含义参考ppt。这一部分会进行写寄存器，内容包括：根据issue和commit的通知修改对应寄存器的Q和V。
        tip: 请注意issue和commit同一个寄存器时的情况
        */
        if (channelToRegfile.commit_rd != -1) {
            regNext.reg[channelToRegfile.commit_rd] = channelToRegfile.commit_value;
/*            if (channelToRegfile.commit_rd==18 && regNext.reg[channelToRegfile.commit_rd]== 131048)
                cout<<"Daweaweawesdqwa\n";*/
/*            if (channelToRegfile.commit_rd==13 && regNext.reg[channelToRegfile.commit_rd]==400673)
                std::cout<<"waeaweaweqa\n";*/
  //         cout<<std::dec<<"reg["<<channelToRegfile.commit_rd<<"] "<<regNext.reg[channelToRegfile.commit_rd]<<std::hex<<std::endl;
            if (regNext.Q[channelToRegfile.commit_rd] == channelToRegfile.id)regNext.Q[channelToRegfile.commit_rd] = 0;
        }
        if (channelToRegfile.issue_rd != -1) regNext.Q[channelToRegfile.issue_rd] = channelToRegfile.issue_pos;
        channelToRegfile.clear();
    }

    void run_commit() {
        /*
        在这一部分你需要完成的工作：
        1. 根据ROB发出的信息通知regfile修改相应的值，包括对应的ROB和是否被占用状态（注意考虑issue和commit同一个寄存器的情况）
        2. 遇到跳转指令更新pc值，并发出信号清空所有部分的信息存储（这条对于很多部分都有影响，需要慎重考虑）
        */
        commit_to_SLB = false;
        commit_flag = false;
        if (!rob.isEmpty()) {
            ROB_Node tmp = rob.getFront();
            if (tmp.hasValue) {/*
                if (tmp.isJump) {
                    if (tmp.origin_code == 0X8067)
                        cout<<"Awedawedaw";
                    cout<<std::hex<<"***"<<tmp.origin_code<<" "<<(tmp.predict_pc==tmp.npc)<<"***"<<std::dec<<std::endl;
                }*/
                commit_to_SLB = tmp.isStore;
                commit_to_SLB_id = tmp.id;
                channelToRegfile.commit_rd = tmp.linkToReg;
                channelToRegfile.commit_value = tmp.value;
                channelToRegfile.id = tmp.id;
                commit_flag = true;
                code_from_rob_to_commit = tmp.origin_code;
                if (tmp.isJump && tmp.predict_pc != tmp.npc) {
                    issueResult.hasResult = false;
                    issue_to_ex_flag = false;
                    reserve_flag = reserve_isStore = reserve_isJump = false;
                    fetch_flag = false;
                    exResult.hasResult = false;
                    rs.exFlag = false;
                    slBuffer.hasResult = false;
                    ROB_is_stall = RS_is_stall = SLBuffer_is_stall = false;
                    rs.clear();
                    rob.clear();
                    slBuffer.clear();
                    preFetchQue.clear();
                    nextFetchQue.clear();
                    regPre.clearQ();
                    regNext.clearQ();
                    channelToRegfile.clear();

                    next_pc = tmp.npc;
                    commit_flag = false;
                }
            }
        }
    }

    void update() {
        /*
        在这一部分你需要完成的工作：
        对于模拟中未完成同步的变量（即同时需记下两个周期的新/旧信息的变量）,进行数据更新。
        */
        regPre = regNext;
        preFetchQue = nextFetchQue;
        rs.update();
        slBuffer.update();
        rob.update();
    }

    ~simulator() {
        delete[] memory;
        while (vec_basePtr.getStatus()!=-1) {
            delete vec_basePtr.getFront();
            vec_basePtr.pop();
        }
    }
};

#endif