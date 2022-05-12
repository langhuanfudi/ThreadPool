//
// Created by hxh on 2022/5/10.
//

#ifndef THREADPOOL_THREADPOOL_H
#define THREADPOOL_THREADPOOL_H

#include <queue>
#include <mutex>
#include <future>

/*************************************************** 任务队列 ***************************************************/
template<typename T>
class SafeQueue {
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
public:
    SafeQueue() {}
    SafeQueue(SafeQueue &&other) {}
    ~SafeQueue() {}

    bool empty() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    int size() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    void enqueue(T &t) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.emplace(t);
    }

    bool dequeue(T &t) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;
        t = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

};

/*************************************************** 线程池 ***************************************************/

class ThreadPool {
private:
    bool m_shutdown; // 线程是否关闭
    SafeQueue<std::function<void()>> m_queue; // 任务队列
    std::vector<std::thread> m_threads;
    std::mutex m_conditional_mutex; // 线程休眠锁, 互斥变量
    std::condition_variable m_conditional_lock; // 线程环境锁, 可以让线程处于休眠或者唤醒状态

    class ThreadWorker {
    private:
        int m_id;
        ThreadPool *m_pool;
    public:
        ThreadWorker(ThreadPool *pool, const int id) : m_pool(pool), m_id(id) {}

        void operator()() {
            std::function<void()> func;
            bool dequeued; // 是否正在取出队列中元素
            while (!m_pool->m_shutdown) {
                std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
                if (m_pool->m_queue.empty())
                    m_pool->m_conditional_lock.wait(lock);
                dequeued = m_pool->m_queue.dequeue(func);
            }
            if (dequeued) func();
        }
    };

public:
    ThreadPool(const int n_threads = 4) : m_threads(std::vector<std::thread>(n_threads)), m_shutdown(false) {}
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    void init() {
        for (int i = 0; i < m_threads.size(); ++i) {
            m_threads.at(i) = std::thread(ThreadWorker(this, i));
        }
    }

    void shutdown() {
        m_shutdown = true;
        m_conditional_lock.notify_all();
        for (int i = 0; i < m_threads.size(); ++i) {
            if (m_threads.at(i).joinable()) {
                m_threads.at(i).join();
            }
        }
    }

    /**
     * 提交函数
     * 接收任何参数的任何函数
     * 立即返回"东西", 避免阻塞主线程, "东西"应该包含任务结束的结果
     */
    template<typename F, typename ... Args>
    auto submit(F &&f, )

};




#endif //THREADPOOL_THREADPOOL_H
