#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <iostream>
#include <chrono>
#include "MultiQueueProcessor.h"

struct  MyConsumer : IConsumer<std::string, int> 
{
    MyConsumer(std::string key) : IConsumer<std::string, int>(key) {}
    ~MyConsumer() {}

    void Consume( const int& value ) {
        std::cout << "I've just consumed a yet another value " << value << std::endl;
        std::this_thread::sleep_for( std::chrono::milliseconds( 250 + (rand() / RAND_MAX) * 500) );
    }
};

struct MyProducer : IProducer<std::string, int>  
{
    static inline int i=0;

    MyProducer(MultiQueueProcessor<std::string, int>& mqp, std::string key) : IProducer<std::string, int>(mqp, key) {}
    ~MyProducer() {}

    int Produce() {
        std::this_thread::sleep_for(std::chrono::milliseconds( 250 + (rand() / RAND_MAX) * 400) );
        std::cout << "I'm producing a yet another value " << i+1 << std::endl;
        return ++i; 
    }
};


int main( void )
{    
    MultiQueueProcessor<std::string, int> mqp;

    MyProducer prod1( mqp, std::string("key1") );
    prod1.runAndDetach();
    MyProducer prod2( mqp, std::string("key2") );
    prod2.runAndDetach();

    MyConsumer cons1("key1");
    MyConsumer cons2("key2");
    mqp.Subscribe( &cons1 );
    mqp.Subscribe( &cons2 );

    std::this_thread::sleep_for( std::chrono::milliseconds( 2000 ) );
    std::cout << "UNSUBSCRIBING!\n\n\n\n" << std::endl;
    mqp.Unsubscribe("key1");

    std::cin.get();  
}