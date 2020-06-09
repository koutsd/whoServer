
void* thread_function(void *arg) {
    while(true) {
        pthread_mutex_lock(&buffer_mutex);
        while(buffer->isEmpty())
            pthread_cond_wait(&buffer_cond, &buffer_mutex);

        int fd = buffer->pop();
        pthread_mutex_unlock(&buffer_mutex);

        pthread_mutex_lock(&workers_mutex);
        bool isWorker = workers->member(fd);
        pthread_mutex_unlock(&workers_mutex);

        if(isWorker) {
            string stats = receiveMessage(fd);
            pthread_mutex_lock(&workers_mutex);
            workers->addData(fd, stats);
            pthread_cond_signal(&workers_cond);
            pthread_mutex_unlock(&workers_mutex);

            pthread_mutex_lock(&buffer_mutex);
            buffer->push(fd);
            pthread_cond_signal(&buffer_cond);
            pthread_mutex_unlock(&buffer_mutex);

            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        bool isPendingClient = clients->member(fd);
        string query = isPendingClient ? clients->getData(fd)->dequeue() : "";
        pthread_mutex_unlock(&clients_mutex);

        if(isPendingClient) {
            pthread_mutex_lock(&clients_mutex);
            while(!clients->isLast(fd))
                pthread_cond_wait(&clients_cond, &clients_mutex);

            clients->remove(fd);
            pthread_mutex_lock(&workers_mutex);
            while(!workers->allHaveData())
                pthread_cond_wait(&workers_cond, &workers_mutex);
            
            string res = "";
            for(int i = 0; i < workers->length(); i++) {
                if(query.find("/diseaseFrequency") != string::npos)
                    res = to_string(stoi(res) + stoi(workers->removeDatafromIndex(i)));
                else
                    res += workers->removeDatafromIndex(i);
            }
            
            pthread_mutex_unlock(&workers_mutex);
            pthread_cond_signal(&clients_cond);
            pthread_mutex_unlock(&clients_mutex);

            cout << query + "\n" + res + "\n";
            sendMessage(fd, res);
        }
        else {
            string query = receiveMessage(fd);

            pthread_mutex_lock(&workers_mutex);

            for(int i = 0; i < queryFD_list->length(); i++)
                sendMessage(queryFD_list->get(i), query);

            pthread_mutex_unlock(&workers_mutex);

            pthread_mutex_lock(&clients_mutex);
            pthread_mutex_lock(&buffer_mutex);
            clients->insert(fd);
            clients->addData(fd, query);
            buffer->push(fd);
            pthread_cond_signal(&buffer_cond);
            pthread_mutex_unlock(&buffer_mutex);
            pthread_mutex_unlock(&clients_mutex);
        }
    }
}