#pragma once
#include <map>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

template<typename Key, typename Value>
class MultiQueueProcessor;

template<typename Key, typename Value>
struct IConsumer
{
    bool m_running=false;
    Key m_key;
    std::thread *m_thread;
    std::mutex mtx;
    std::condition_variable cond;
    bool b_value=false;     // To signal a new value comes
    Value m_value;

    IConsumer(Key key) : m_key(key) {}

    ~IConsumer() {
        m_running = false;
        delete m_thread;
    }

    void stop() {
        m_running = false;
    }

    void runAndDetach() {
        m_running = true;
        m_thread = new std::thread( &IConsumer::consumeThread, this );
        m_thread->detach();
    }

    void consumeThread() {
        while(m_running) {
            std::unique_lock<std::mutex> L{mtx};
            cond.wait( L, [&]()
            {
                return b_value;
            });        
            b_value = false;
            Consume(m_value);
        }
    }

    virtual void Consume(const Value &value) = 0; // {}
};

template <typename Key, typename Value>
struct IProducer
{
    bool m_running = false;
    MultiQueueProcessor<Key,Value>& m_mqp;
    Key m_key;
    std::thread *m_thread;

    IProducer(MultiQueueProcessor<Key, Value>& mqp, Key key) : m_mqp(mqp), m_key(key) {}
    ~IProducer() {
        m_running = false;
        delete m_thread;
    }

    void stop() {
        m_running = false;
    }

    void runAndDetach() {
        m_running = true;
        m_thread = new std::thread( &IProducer::producerThread, this );
        m_thread->detach();
    }

    void producerThread() {
        while(m_running) {
            //std::cout << "A Producer thread has started!" << std::endl;
            Value value = Produce();
            if( !m_running ) {  // An crirical error signalled!
                break;
            }
            m_mqp.Enqueue( m_key, value );
        }
    }

	virtual Value Produce()=0; // To implement in a derived class
};


#define MaxCapacity 1000

template<typename Key, typename Value>
class MultiQueueProcessor
{
protected:
	std::map<Key, IConsumer<Key, Value> *> consumers;   // key - is a queue key
	std::map<Key, std::list<Value>> queues;

	bool running;
	std::mutex mtx;
    std::condition_variable cond;
	std::thread *m_thread;

public:
	MultiQueueProcessor() : running{ true }
    {
	    m_thread = new std::thread( &MultiQueueProcessor::Process, this );
        m_thread->detach();
    }

	~MultiQueueProcessor()
	{
		delete m_thread;
		running = false;
	}

	void Subscribe(IConsumer<Key, Value> * consumer)
	{
		std::lock_guard<std::mutex> lock{ mtx };
		auto iter = consumers.find( consumer->m_key );
		if (iter == consumers.end())
		{
            consumer->runAndDetach();
			consumers.insert(std::make_pair(consumer->m_key, consumer));
		}
	}

	void Unsubscribe(Key id)
	{
		std::lock_guard<std::mutex> lock{ mtx };
		auto iter = consumers.find(id);
		if (iter != consumers.end())
			consumers.erase(id);
	}

	void Enqueue(Key id, Value value)
	{
        if( !running ) {
            return;
        }
		std::lock_guard<std::mutex> lock{ mtx };
        // std::cout << "Placing into the queue: Key=" << id << ", value=" << value << std::endl;
		auto iter = queues.find(id);
		if (iter != queues.end())
		{
			if (iter->second.size() < MaxCapacity) { 
				iter->second.push_back(value);
            }
		}
		else
		{
			queues.insert(std::make_pair(id, std::list<Value>()));
			iter = queues.find(id);
			if (iter != queues.end())
			{
				if (iter->second.size() < MaxCapacity) {
					iter->second.push_back(value);
                }
			}
		}
        cond.notify_one();
	}

	Value Dequeue(Key id)
	{
		//std::lock_guard<std::mutex> lock{ mtx };
		auto iter = queues.find(id);
		if (iter != queues.end())
		{
			if (iter->second.size() > 0)
			{
				auto front = iter->second.front();
				iter->second.pop_front();
				return front;
			}
		}
		return Value{};
	}

protected:
	void Process()
	{
		while (running)
		{
			std::unique_lock<std::mutex> lock{ mtx };      
            std::map<Key, std::list<Value>>::iterator queueIter;
            std::map<Key, IConsumer<Key, Value> *>::iterator consumerIter;
            cond.wait( lock, [&]()
            {
    			for (queueIter = queues.begin(); queueIter != queues.end(); ++queueIter)
	    		{
		    		consumerIter = consumers.find(queueIter->first);    // Is there a consumer for the queue?  
			    	if (consumerIter != consumers.end())                // If there is one...
				    {
                        if( consumerIter->second->m_running ) {
                            if( queueIter->second.size() > 0 ) {
                                return true;
                            }
                        }
                    }
                }
                return false;
            });

            Value front = queueIter->second.front();
            queueIter->second.pop_front();
            std::lock_guard<std::mutex> consumerLock{ consumerIter->second->mtx };
            consumerIter->second->m_value = front;
            consumerIter->second->b_value = true;
            consumerIter->second->cond.notify_one();
        }
    }
};