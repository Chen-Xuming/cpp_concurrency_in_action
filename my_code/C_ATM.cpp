//
// Created by chen on 2022/8/26.
//

#include <condition_variable>
#include <iostream>
#include <memory>
#include <queue>
#include <thread>
#include <utility>

// -----------------------------------------------
//  消息队列
// -----------------------------------------------
namespace Messaging{
    struct MessageBase{
        virtual ~MessageBase() = default;
    };

    // 消息封装类: 消息队列中的元素
    template<class Msg>
    struct WrappedMessage: MessageBase{
        Msg contents;
        explicit WrappedMessage(const Msg& msg): contents(msg){}
    };

    // 消息队列
    class MessageQueue{
    private:
        std::mutex m_;
        std::condition_variable cv_;
        std::queue<std::shared_ptr<MessageBase>> q_;

    public:
        // 有消息进来，就通知所有等待线程
        template<class T>
        void push(const T& msg){
            std::lock_guard<std::mutex> l(m_);
            q_.push(std::make_shared<WrappedMessage<T>>(msg));
            cv_.notify_all();
        }

        // wait_and_pop: 合并front()和pop()，详见threadsafe_queue的解析
        std::shared_ptr<MessageBase> wait_and_pop(){
            std::unique_lock<std::mutex> l(m_);
            cv_.wait(l, [&]{return !q_.empty();});
            auto res = q_.front();
            q_.pop();
            return res;
        }
    };

} // namespace Messaging

// -----------------------------------------------
//  调度器模板
// -----------------------------------------------
namespace Messaging{
    template<class PreviousDispatcher, class Msg, class F>      // F是函数指针
    class TemplateDispatcher{
    private:
        MessageQueue *q_ = nullptr;
        PreviousDispatcher* prev_ = nullptr;
        F f_;
        bool chained_ = false;

    public:
        TemplateDispatcher(const TemplateDispatcher&) = delete;
        TemplateDispatcher& operator=(const TemplateDispatcher) = delete;

        TemplateDispatcher(TemplateDispatcher &&rhs) noexcept: q_(rhs.q_), prev_(rhs.prev_), f_(std::move(rhs.f_)), chained_(rhs.chained_){
            rhs.chained_ = true;
        }

        TemplateDispatcher(MessageQueue *q, PreviousDispatcher *prev, F &&f)
                :q_(q), prev_(prev), f_(std::forward<F>(f)){
            prev->chained_ = true;
        }

        // 按连接成链的方式引入多个处理函数
        template<class OtherMsg, class OtherF>
        TemplateDispatcher<TemplateDispatcher, OtherMsg, OtherF> handle(OtherF &&f){
            return TemplateDispatcher<TemplateDispatcher, OtherMsg, OtherF>(q_, this, std::forward<OtherF>(f));
        }

        ~TemplateDispatcher() noexcept(false){
            if(!chained_){
                wait_and_dispatch();        // 在析构函数中进行任务调度
            }
        }

    private:
        template<class Dispatcher, class OtherMsg, class OtherF>
        friend class TemplateDispatcher;    // TemplateDispatcher 实例互为友元

        void wait_and_dispatch(){
            while(true){
                auto msg = q_->wait_and_pop();
                if(dispatch(msg)){      // 如果消息已经处理妥当，则跳出循环
                    break;
                }
            }
        }

        bool dispatch(const std::shared_ptr<MessageBase> &msg){
            // 检查消息类型并调用相应的处理函数
            if(auto wrapper = dynamic_cast<WrappedMessage<Msg>*>(msg.get())){
                f_(wrapper->contents);
                return true;
            }
            // 如果消息类型不匹配，则连接到前一个 dispatcher，形成连锁调用
            return prev_->dispatch(msg);
        }
    };
}

// -----------------------------------------------
//  调度器类
// -----------------------------------------------
namespace Messaging{
    class CloseQueue{};     // 用于关闭队列的消息

    class Dispatcher{
    private:
        // 允许TemplateDispatcher访问Dispatcher内部数据
        template<class Dispatcher, class Msg, class F>
        friend class TemplateDispatcher;

        void wait_and_dispatch(){
            while(true){
                auto msg = q_->wait_and_pop();
                dispatch(msg);
            }
        }

        bool dispatch(const std::shared_ptr<MessageBase> &msg){
            if(dynamic_cast<WrappedMessage<CloseQueue>*>(msg.get())){   // 转化成功说明 msg 是 WrappedMessage<CloseQueue>*
                throw CloseQueue();
            }
            return false;   // 表示消息未被处理
        }

    private:
        MessageQueue *q_ = nullptr;
        bool chained_ = false;

    public:
        Dispatcher(const Dispatcher&) = delete;
        Dispatcher& operator=(const Dispatcher&) = delete;
        Dispatcher(Dispatcher&& rhs) noexcept: q_(rhs.q_), chained_(rhs.chained_){
            rhs.chained_ = true;    // 上游的消息分发者不会等待消息
        }
        explicit Dispatcher(MessageQueue *q): q_(q){}

        // 用 TemplateDispatcher 处理特定类型的消息
        template <class Msg, class F>
        TemplateDispatcher<Dispatcher, Msg, F> handle(F &&f){
            return TemplateDispatcher<Dispatcher, Msg, F>(q_, this, std::forward<F>(f));
        }

        ~Dispatcher() noexcept(false){  // 可能抛出 CloseQueue 异常
            if(!chained_){               // 从 Receiver::wait 返回的 dispatcher 实例会马上被析构
                wait_and_dispatch();    // 析构函数中完成任务调度
            }
        }
    };
}

// -----------------------------------------------
//  发送者类
//  消息由它的实例发送。实际上是一个轻量化包装的队列，只允许添加消息。
// -----------------------------------------------
namespace Messaging{
    class Sender{
    private:
        MessageQueue *q_ = nullptr;
    public:
        Sender() = default;
        explicit Sender(MessageQueue *q): q_(q){}  // 复制Sender类实例仅仅复制指向队列容器的指针

        template<class Msg>
        void send(const Msg &msg){
            if(q_){
                q_->push(msg);
            }
        }
    };
}

// -----------------------------------------------
//  接收者类
//  等待消息队列中出现消息，不同种类的消息需要使用不同函数处理，因此需要检查消息类型
// -----------------------------------------------
namespace Messaging{

    class Receiver{
    private:
        MessageQueue q_;    // Sender类仅引用消息队列，而Receiver真正拥有消息队列
    public:
        operator Sender(){
            return Sender(&q_);     // receiver对象隐式转换为sender对象，前者拥有的队列被后者引用
        }

        Dispatcher wait(){
            return Dispatcher(&q_);     // 等待调度：创建一个Dispatcher对象
        }
    };
}

// -----------------------------------------------
//  ATM消息
// -----------------------------------------------
struct Withdraw{
    std::string account;
    unsigned amount;
    mutable Messaging::Sender atm_queue;
    Withdraw(const std::string &_account, unsigned _amount, Messaging::Sender _atm_queue):
                account(_account), amount(_amount), atm_queue(_atm_queue){}
};

struct WithdrawOK{};

struct WithdrawDenied{};

struct CancelWithdrawal{
    std::string account;
    unsigned amount;
    CancelWithdrawal(const std::string &_account, unsigned _amount):
            account(_account), amount(_amount){}
};

struct WithdrawProcessed{
    std::string account;
    unsigned amount;
    WithdrawProcessed(const std::string &_account, unsigned _amount):
            account(_account), amount(_amount){}
};

struct CardInserted{
    std::string account;
    explicit CardInserted(const std::string &_account): account(_account){}
};

struct DigitPressed{
    char digit;
    explicit DigitPressed(char _digit): digit(_digit){}
};

struct ClearLastPressed{};

struct EjectCard{};

struct WithdrawPressed{
    unsigned amount;
    explicit WithdrawPressed(unsigned _amount): amount(_amount){}
};

struct CancelPressed {};

struct IssueMoney {
    IssueMoney(unsigned _amount) : amount(_amount) {}
    unsigned amount;
};

struct VerifyPIN {
    VerifyPIN(const std::string& _account, const std::string& pin_,
              Messaging::Sender _atm_queue)
            : account(_account), pin(pin_), atm_queue(_atm_queue) {}

    std::string account;
    std::string pin;
    mutable Messaging::Sender atm_queue;
};

struct PINVerified {};

struct PINIncorrect {};

struct DisplayEnterPIN {};

struct DisplayEnterCard {};

struct DisplayInsufficientFunds {};

struct DisplayWithdrawalCancelled {};

struct DisplayPINIncorrectMessage {};

struct DisplayWithdrawalOptions {};

struct GetBalance {
    GetBalance(const std::string& _account, Messaging::Sender _atm_queue)
            : account(_account), atm_queue(_atm_queue) {}

    std::string account;
    mutable Messaging::Sender atm_queue;
};

struct Balance {
    explicit Balance(unsigned _amount) : amount(_amount) {}

    unsigned amount;
};

struct DisplayBalance {
    explicit DisplayBalance(unsigned _amount) : amount(_amount) {}

    unsigned amount;
};

struct BalancePressed {};


// -----------------------------------------------
//  ATM状态机
// -----------------------------------------------
class ATM{
public:
    ATM(const ATM&) = delete;
    ATM& operator=(const ATM&) = delete;
    ATM(Messaging::Sender bank, Messaging::Sender interface_hardware):
            bank_(bank), interface_hardware_(interface_hardware){}

    Messaging::Sender get_sender(){
        return incoming_;           // receiver 隐式转化为 sender
    }

    void done(){
        get_sender().send(Messaging::CloseQueue());
    }

    void run(){
        state_ = &ATM::waiting_for_card;       // 置最初的状态为等待插卡
        try{
            while(true){
                (this->*state_)();
            }
        }catch (const Messaging::CloseQueue&){}
    }

private:
    Messaging::Receiver incoming_;
    Messaging::Sender bank_;
    Messaging::Sender interface_hardware_;
    void (ATM::*state_)();      // ATM::state_ ====> 类成员函数指针
    std::string account_;
    unsigned withdrawal_amount_;
    std::string pin_;

    void process_withdrawal(){
        incoming_.wait()
                .handle<WithdrawOK>([&](const WithdrawOK &msg){
                    interface_hardware_.send(IssueMoney(withdrawal_amount_));
                    bank_.send(WithdrawProcessed(account_, withdrawal_amount_));
                    state_ = &ATM::done_processing;
                })
                .handle<WithdrawDenied>([&](const WithdrawDenied &msg){
                    interface_hardware_.send(DisplayInsufficientFunds());
                    state_ = &ATM::done_processing;
                })
                .handle<CancelPressed>([&](const CancelPressed &msg){
                    bank_.send(CancelWithdrawal(account_, withdrawal_amount_));
                    interface_hardware_.send(DisplayWithdrawalCancelled());
                    state_ = &ATM::done_processing;
                });
    }

    void process_balance(){
        incoming_.wait()
                .handle<Balance>([&](const Balance &msg){
                    interface_hardware_.send(DisplayBalance(msg.amount));
                    state_ = &ATM::wait_for_action;
                })
                .handle<CancelPressed>([&](const CancelPressed &msg){
                    state_ = &ATM::done_processing;
                });
    }

    void wait_for_action(){
        interface_hardware_.send(DisplayWithdrawalOptions());
        incoming_.wait()
                .handle<WithdrawPressed>([&](const WithdrawPressed &msg){
                    withdrawal_amount_ = msg.amount;
                    bank_.send(Withdraw(account_, msg.amount, incoming_));
                    state_ = &ATM::process_withdrawal;
                })
                .handle<BalancePressed>([&](const BalancePressed &msg){
                    bank_.send(GetBalance(account_, incoming_));
                    state_ = &ATM::process_balance;
                })
                .handle<CancelPressed>([&](const CancelPressed &msg){
                    state_ = &ATM::done_processing;
                });
    }

    void verifying_pin(){
        incoming_.wait()
                .handle<PINVerified>([&](const PINVerified &msg){
                    state_ = &ATM::wait_for_action;
                })
                .handle<PINIncorrect>([&](const PINIncorrect &msg){
                    interface_hardware_.send(DisplayPINIncorrectMessage());
                    state_ = &ATM::done_processing;
                })
                .handle<CancelPressed>([&](const CancelPressed &msg){
                    state_ = &ATM::done_processing;
                });
    }

    void getting_pin(){
        incoming_.wait()
                .handle<DigitPressed>([&](const DigitPressed &msg){
                    const unsigned pin_length = 4;
                    pin_ += msg.digit;
                    if(pin_.length() == pin_length){
                        bank_.send(VerifyPIN(account_, pin_, incoming_));
                        state_ = &ATM::verifying_pin;
                    }
                })
                .handle<ClearLastPressed>([&](const ClearLastPressed &msg){
                    if(!pin_.empty()){
                        pin_.pop_back();
                    }
                })
                .handle<CancelPressed>([&](const CancelPressed &msg){
                    state_ = &ATM::done_processing;
                });
    }

    void waiting_for_card(){
        interface_hardware_.send(DisplayEnterCard());
        incoming_.wait().handle<CardInserted>([&](const CardInserted &msg){
            account_ = msg.account;
            pin_ = "";
            interface_hardware_.send(DisplayEnterPIN());
            state_ = &ATM::getting_pin;
        });
    }

    void done_processing(){
        interface_hardware_.send(EjectCard());
        state_ = &ATM::waiting_for_card;
    }
};

// -----------------------------------------------
//  银行状态机
// -----------------------------------------------
class BankMachine{
private:
    Messaging::Receiver incoming_;
    unsigned balance_ = 199;

public:
    void run(){
        try{
            while(true){
                incoming_.wait()
                        .handle<VerifyPIN>([&](const VerifyPIN &msg){
                            if(msg.pin == "6666"){
                                msg.atm_queue.send(PINVerified());
                            }else{
                                msg.atm_queue.send(PINIncorrect());
                            }
                        })
                        .handle<Withdraw>([&](const Withdraw &msg){
                            if(balance_ >= msg.amount){
                                msg.atm_queue.send(WithdrawOK());
                                balance_ -= msg.amount;
                            }else{
                                msg.atm_queue.send(WithdrawDenied());
                            }
                        })
                        .handle<GetBalance>([&](const GetBalance &msg){
                            msg.atm_queue.send(Balance(balance_));
                        })
                        .handle<WithdrawProcessed>([&](const WithdrawProcessed &msg){})
                        .handle<CancelWithdrawal>([&](const CancelWithdrawal &msg){});
            }
        }catch (const Messaging::CloseQueue&){}
    }

    void done(){
        get_sender().send(Messaging::CloseQueue());
    }

    Messaging::Sender get_sender(){
        return incoming_;
    }
};


// -----------------------------------------------
//  用户接口状态机
// -----------------------------------------------
class InterfaceMachine{
private:
    Messaging::Receiver incoming_;
    std::mutex m_;
public:
    void done() { get_sender().send(Messaging::CloseQueue()); }
    Messaging::Sender get_sender() { return incoming_; }

    void run(){
        try{
            while(true){
                incoming_.wait()
                        .handle<IssueMoney>([&](const IssueMoney &msg){
                            std::lock_guard<std::mutex> l(m_);
                            std::cout << "Issuing " << msg.amount << std::endl;
                        })
                        .handle<DisplayInsufficientFunds>([&](const DisplayInsufficientFunds &msg){
                            std::lock_guard<std::mutex> l(m_);
                            std::cout << "Insufficient funds" << std::endl;
                        })
                        .handle<DisplayEnterPIN>([&](const DisplayEnterPIN& msg) {
                            {
                                std::lock_guard<std::mutex> l(m_);
                                std::cout << "Please enter your PIN (0-9)" << std::endl;
                            }
                        })
                        .handle<DisplayEnterCard>([&](const DisplayEnterCard& msg) {
                            {
                                std::lock_guard<std::mutex> l(m_);
                                std::cout << "Please enter your card (I)" << std::endl;
                            }
                        })
                        .handle<DisplayBalance>([&](const DisplayBalance& msg) {
                            {
                                std::lock_guard<std::mutex> l(m_);
                                std::cout << "The Balance of your account is " << msg.amount
                                          << std::endl;
                            }
                        })
                        .handle<DisplayWithdrawalOptions>(
                                [&](const DisplayWithdrawalOptions& msg) {
                                    {
                                        std::lock_guard<std::mutex> l(m_);
                                        std::cout << "Withdraw 50? (w)" << std::endl;
                                        std::cout << "Display Balance? (b)" << std::endl;
                                        std::cout << "Cancel? (c)" << std::endl;
                                    }
                                })
                        .handle<DisplayWithdrawalCancelled>(
                                [&](const DisplayWithdrawalCancelled& msg) {
                                    {
                                        std::lock_guard<std::mutex> l(m_);
                                        std::cout << "Withdrawal cancelled" << std::endl;
                                    }
                                })
                        .handle<DisplayPINIncorrectMessage>(
                                [&](const DisplayPINIncorrectMessage& msg) {
                                    {
                                        std::lock_guard<std::mutex> l(m_);
                                        std::cout << "PIN incorrect" << std::endl;
                                    }
                                })
                        .handle<EjectCard>([&](const EjectCard& msg) {
                            {
                                std::lock_guard<std::mutex> l(m_);
                                std::cout << "Ejecting card" << std::endl;
                            }
                        });
            }
        }catch (Messaging::CloseQueue&){}
    }
};

// 驱动程序
int main(){
    BankMachine bank;
    InterfaceMachine interface_hardware;
    ATM machine(bank.get_sender(), interface_hardware.get_sender());
    std::thread bank_thread(&BankMachine::run, &bank);
    std::thread interface_thread(&InterfaceMachine::run, &interface_hardware);
    std::thread atm_thread(&ATM::run, &machine);
    Messaging::Sender atm_queue(machine.get_sender());

    // 主线程
    bool quit = false;
    while(!quit){
        char c = getchar();
        switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                atm_queue.send(DigitPressed(c));
                break;
            case 'b':       // 显示余额
                atm_queue.send(BalancePressed());
                break;
            case 'w':       // 取钱
                atm_queue.send(WithdrawPressed(50));
                break;
            case 'c':       // 退卡
                atm_queue.send(CancelPressed());
                break;
            case 'i':       // 插卡
                atm_queue.send(CardInserted("chen"));
                break;
            case 'q':       // 结束
                quit = true;
                break;
            default:
                break;
        }
    }

    bank.done();
    machine.done();
    interface_hardware.done();
    atm_thread.join();
    bank_thread.join();
    interface_thread.join();

    return 0;
}