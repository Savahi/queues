#include <string>
#include <iostream>
#include <chrono>
#define _WIN32_WINNT 0x501
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "MultiQueueProcessor.h"
#include "tcpacceptor.h"

static constexpr char _http_response[] = "HTTP/1.1 200 OK\r\nVersion: HTTP/1.1\r\nContent-Type:text/html\r\nContent-Length:2\r\n\r\nOk";
static constexpr char _root_response[] = "HTTP/1.1 200 OK\r\nVersion: HTTP/1.1\r\nContent-Type:text/html\r\n\r\n<html><head><link rel=\"shortcut icon\" href=\"data:image/x-icon;,\" type=\"image/x-icon\"></head><body><div onclick='test()' style='border:1px solid gray; cursor:pointer; width:200px; text-align:center; padding:8px;'>Click here to test!</div><div id='testLog'></div><script>var testLogId = document.getElementById('testLog');function test() { for( let i = 0 ; i < 100 ; i++ ) { fetch('/test', { method: 'get' } ).then( response => response.text() ).then( function(response) { testLogId.innerHTML += i + ':' + response + ' +</br>'; } ).catch(err => { testLogId.innerHTML += i + ': <span style=\"color:red;\">ERROR!</span></br>'; } ); } }</script></body></html>";

struct  MyConsumer : IConsumer<std::string, int> 
{
    MyConsumer(std::string key) : IConsumer<std::string, int>(key) {}
    ~MyConsumer() {}

    void Consume( const int& client_socket ) {
        char _socket_request_buf[1024*10];
    
        std::cout << "Handler: client socket=" << client_socket << std::endl;
        int result = recv(client_socket, _socket_request_buf, sizeof(_socket_request_buf)-1, 0);
        if( result == 0 || result == SOCKET_ERROR )
            return;
        //std::cout << "Client sent: " << _socket_request_buf << std::endl;    
        if( strncmp( _socket_request_buf, "GET / ", 6) == 0 ) {
            send(client_socket, _root_response, strlen(_root_response), 0);        
        } else {
            send(client_socket, _http_response, strlen(_http_response), 0);
        }
        closesocket(client_socket);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
};

struct MyProducer : IProducer<std::string, int>, TCPAcceptor  
{    
    MyProducer( 
        MultiQueueProcessor<std::string, int>& mqp, std::string key,std::string ip, std::string port ) : 
        IProducer<std::string, int>(mqp, key), 
        TCPAcceptor(ip, port) {
    }
    ~MyProducer() {}

    int Produce() {
        //std::this_thread::sleep_for(std::chrono::milliseconds( 250 + (rand() / RAND_MAX) * 400) );
        if( !m_listening ) {
            start();
        }
        if( !m_listening ) {
            m_running = false;
            return -1;
        }
        return run();
        /*
        int client_socket = accept();
        if( client_socket == -1 ) {
            m_running = false;
        }
        std::cout << "I'm producing a new client socket: " << client_socket << std::endl;
        return client_socket;
        */
    }
};


int main( void )
{    
    MultiQueueProcessor<std::string, int> mqp;

    MyProducer prod1( mqp, std::string("localhost8000"), std::string("localhost"), std::string("8000") );
    prod1.runAndDetach();
    MyProducer prod2( mqp, std::string("localhost8001"), std::string("localhost"), std::string("8001") );
    prod2.runAndDetach();

    MyConsumer cons1("localhost8000");
    MyConsumer cons2("localhost8001");
    mqp.Subscribe( &cons1 );
    mqp.Subscribe( &cons2 );

    std::cin.get();  
}