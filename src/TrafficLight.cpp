#include <iostream>
#include <random>
#include <future>
#include <queue>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{   
    /* Create a lock and pass it to the condition variable */
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this]{ return !_queue.empty(); });
    
    /* Get the latest element and remove it from the queue */
    T msg = std::move(_queue.back());
    _queue.pop_back();
    
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T&& msg)
{
    /* Prevent data race */
    std::lock_guard<std::mutex> uLock(_mutex);
  
    /* Move into queue and notify client */
    _queue.push_back( std::move(msg));
    _cond.notify_one();
}

/* Implementation of class "TrafficLight" */
 
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _msgQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true)
    {
        /* Sleep at every iteration to reduce CPU usage*/
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        /* Wait until the traffic light is green, received from message queue */
        auto currPhase = _msgQueue->receive();
        if (currPhase == green)
            return;
    }
      
}

TrafficLightPhase TrafficLight::getCurrentPhase() const
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
    
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(4,6);
  
    /* Print id of the current thread */
    std::unique_lock<std::mutex> lck(_mutex);
    std::cout << "Traffic Light #" << _id << " Cycle Through Phases: thread id = "<<std::this_thread::get_id() <<std::endl;
    lck.unlock();
  
    /* Initialize variables */
    int cycleDuration = distr(eng);
  
    /* Init stop watch */
    auto last_update = std::chrono::system_clock::now();
  
    while(true)
    {
        /* Compute time difference to stop watch */
        long time_since_last_update = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - last_update).count();
      
        /* Sleep at every iteration to reduce CPU usage */
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      
        /* It is time to toggle our traffic light */
        if (time_since_last_update >= cycleDuration)
        {
            /* Toggle current phase of traffic light */
            if (_currentPhase == red)
                _currentPhase = green;
            else
                _currentPhase = red;
          
            /* Send an update to the message queue and wait for it to be sent */
            auto msg = _currentPhase;
            auto isSent = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _msgQueue, std::move(msg));
            isSent.wait();
            
            /* Reset stop watch for next cycle */
            last_update = std::chrono::system_clock::now();
          
            /* Randomly choose the cycle duration for the next cycle */
            cycleDuration = distr(eng);
        }
    }
}

