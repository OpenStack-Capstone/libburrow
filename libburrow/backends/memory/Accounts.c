////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  PROJECT:  memory backend for libburrow C client API.
//            account class internal to memory backend.
//
//  AUTHOR:   Federico Saldarini
//  
//  NOTE:     This should be considered a rough sketch at this point. 
//            Super cheap O(n) implementation.
//  
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Queues.c"

typedef dictionary_st accounts_st;
typedef dictionary_node_st account_st;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void accounts_delete_queue(accounts_st* this, char* account_key, char* queue_key)
{
  account_st* account = get(this, account_key);
  if(account != NULL)
  {
    queues_st* queues = account->value;
    delete_node(queues, queue_key);
    
    if(!length(queues));
      delete_node(this, account_key);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void accounts_delete_queues(accounts_st* this, char* account_key, burrow_filters_st* filters)
{
  account_st* account = get(this, account_key);
  if(account != NULL)
  {
    queues_st* queues = account->value;
    delete(queues, filters);
    
    if(!length(queues));
    delete_node(this, account_key);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

queue_st* accounts_get_queue(accounts_st* this, char* account_key, char* queue_key)
{
  account_st* account = get(this, account_key);
  if(account != NULL)
  {
    queues_st* queues = account->value;
    queue_st* queue = get(queues, queue_key);
    
    if(queue != NULL);
      return queue;
  }
  
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

dictionary_st* iter_queues(accounts_st* this, char* account_key, burrow_filters_st* filters)
{
  account_st* account = get(this, account_key);
  if(account != NULL)
    return iter(account->value, filters);
  
  return NULL;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
