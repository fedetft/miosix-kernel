
//
// Key API (thread local storage)
//

struct KeyEntries
{
    void *key;
    miosix::Thread *owner;
    KeyEntries *next;
};

struct KeyTable
{
    KeyEntries *entries[4];
    void (*dtor[4])(void *);
    unsigned char used; ///First 4 bits used as bitmap
    KeyTable *next; ///For more than 4 keys, linked list
};

static KeyTable *root=0;
static pthread_mutex_t keyMutex=PTHREAD_MUTEX_INITIALIZER;

int	pthread_key_create(pthread_key_t *key, void (*dtor)(void *))
{
    pthread_mutex_lock(&keyMutex);
    if(root==0)
    {
        root=(KeyTable*)malloc(sizeof(KeyTable));
        if(root==0)
        {
            pthread_mutex_unlock(&keyMutex);
            return EAGAIN;
        }
        root->used=1;
        root->entries[0]=0;
        root->dtor[0]=dtor;
        *key=1;
        pthread_mutex_unlock(&keyMutex);
        return 0;
    }

    unsigned int id=1;
    KeyTable *walk=root;
    for(;;)
    {
        if(walk->used!=0xf)
        {
            for(int i=0;i<4;i++,id++)
            {
                if(walk->used & (1<<i)) continue;
                walk->used |= 1<<i;
                walk->entries[i]=0;
                walk->dtor[i]=dtor;
                *key=id;
                pthread_mutex_unlock(&keyMutex);
                return 0;
            }
        } else id+=4;

        if(walk->next==0)
        {
            walk->next=(KeyTable*)malloc(sizeof(KeyTable));
            if(walk->next==0)
            {
                pthread_mutex_unlock(&keyMutex);
                return EAGAIN;
            }
            walk->next->used=1;
            walk->next->entries[0]=0;
            walk->next->dtor[0]=dtor;
            *key=id;
            pthread_mutex_unlock(&keyMutex);
            return 0;
        }
        
        walk=walk->next;
    }
}

int	pthread_setspecific(pthread_key_t key, const void *value)
{
    if(key==0) return EINVAL;
    key--;
    pthread_mutex_lock(&keyMutex);
    if(root==0)
    {
        pthread_mutex_unlock(&keyMutex);
        return EINVAL;
    }
    KeyTable *walk=root;
    while(key>=4)
    {
        key-=4;
        if(walk->next==0)
        {
            pthread_mutex_unlock(&keyMutex);
            return EINVAL;
        }
        walk=walk->next;
    }

    if((walk->used & (1<<key))==0)
    {
        pthread_mutex_unlock(&keyMutex);
        return EINVAL;
    }
    KeyEntries *keys=walk->entries[key];

    //TODO

    pthread_mutex_unlock(&keyMutex);
}

void *pthread_getspecific(pthread_key_t key)
{

}

int	pthread_key_delete(pthread_key_t key)
{
    
} 
