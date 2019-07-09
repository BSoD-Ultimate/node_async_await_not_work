#include <nan.h>
#include <memory>
#include <thread>
#include <set>

// simple wrapper for uv_async_t (maybe wrong?)
class uvAsyncEvent
{
public:
    uvAsyncEvent(void* context, uv_async_cb callback)
    {
        m_pAsyncHandle.reset(new uv_async_t());
        m_pAsyncHandle->data = context;
        uv_async_init(uv_default_loop(), m_pAsyncHandle.get(), callback);
    }
    ~uvAsyncEvent()
    {

    }
    void* GetContext() const
    {
        return m_pAsyncHandle->data;
    }
    void Notify()
    {
        uv_async_send(m_pAsyncHandle.get());
    }

private:
    struct Deleter
    {
        void operator()(uv_async_t* asyncEvent) const
        {
            if (asyncEvent)
            {
                uv_close((uv_handle_t*)asyncEvent, Deleter::uvCloseCallback);
            }
        }

        static void uvCloseCallback(uv_handle_t* handle)
        {
            delete (uv_async_t*)handle;
        }
    };
    std::unique_ptr<uv_async_t, Deleter> m_pAsyncHandle;
};

class Foo
{
    friend class FooManager;
private:
    Foo()
        : m_pAsync(std::make_unique<uvAsyncEvent>(this, uvAsyncCallback))
    {
    }

    static void uvAsyncCallback(uv_async_t* h)
    {
    }

public:
    ~Foo()
    {
    }

private:
    std::unique_ptr<uvAsyncEvent> m_pAsync;
};

// "Foo" Manager
class FooManager
{
    FooManager()
    {
    }
public:
    ~FooManager()
    {
    }

    static FooManager& GetInstance()
    {
        static FooManager m;
        return m;
    }

    Foo* CreateFoo()
    {
        std::unique_ptr<Foo> pFoo(new Foo());
        Foo* ret = pFoo.get();
        m_createdInstances.insert(std::move(pFoo));
        return ret;
    }

private:
    std::set<std::unique_ptr<Foo>> m_createdInstances;
};

// a c++ class exported to Node.js which holds a "context like" object
class Hang
    : public Nan::ObjectWrap
{
public:
    static NAN_MODULE_INIT(Init)
    {
        v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
        tpl->SetClassName(Nan::New("Hang").ToLocalChecked());
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());

        Nan::Set(target, Nan::New("Hang").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
    }

private:
    Hang()
        //: m_pFoo(FooManager::GetInstance().CreateFoo())
        : m_pAsync(std::make_unique<uvAsyncEvent>(this, uvAsyncCallback))
    {
    }
public:
    ~Hang()
    {
    }

    static void uvAsyncCallback(uv_async_t* h)
    {
    }

    static NAN_METHOD(New)
    {
        if (info.IsConstructCall()) {
            // Invoked as constructor: `new Hang(...)`
            Hang* obj = new Hang();
            obj->Wrap(info.This());

            info.GetReturnValue().Set(info.This());
        }
        else {
            // Invoked as plain function `Hang(...)`, turn into construct call.
            const int argc = 1;
            v8::Local<v8::Value> argv[argc] = { info[0] };
            v8::Local<v8::Function> cons = Nan::New(constructor);
            v8::Local<v8::Object> result =
                Nan::NewInstance(cons, argc, argv).ToLocalChecked();
            info.GetReturnValue().Set(result);
        }
    }

private:
    //Foo* m_pFoo;
    std::unique_ptr<uvAsyncEvent> m_pAsync;

private:
    static Nan::Persistent<v8::Function> constructor;
};
Nan::Persistent<v8::Function> Hang::constructor;

// an asynchronous function for test
NAN_METHOD(testAsync)
{
    class Worker : public Nan::AsyncWorker
    {
    public:
        Worker(Nan::Callback* callbk)
            : Nan::AsyncWorker(callbk)
        {
        }

        void Execute() override
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        void HandleOKCallback() override
        {
            Nan::HandleScope scope;

            int argc = 1;
            std::unique_ptr<v8::Local<v8::Value>[]> argv(new v8::Local<v8::Value>[argc]());

            argv[0] = Nan::New("async task complete").ToLocalChecked();

            Nan::Call(*callback, argc, argv.get());
        }
    };

    Nan::Callback* callbk = new Nan::Callback(Nan::To<v8::Function>(info[0]).ToLocalChecked());

    Worker* pWorker = new Worker(callbk);
    Nan::AsyncQueueWorker(pWorker);
}

NAN_MODULE_INIT(InitAll) 
{
    Hang::Init(target);
    Nan::SetMethod(target, "testAsync", testAsync);
}

NODE_MODULE(addon, InitAll)

