#ifndef __BURROW_GENERIC_TESTS_H
#define __BURROW_GENERIC_TESTS_H

typedef enum {
  EXPECT_NONE = 0,
  
  CALL_QUEUE        = (1 << 0),
  MATCH_QUEUE_ONLY  = (1 << 1),
  MULT_QUEUE_ONLY   = (1 << 2),
  MATCH_QUEUE       = CALL_QUEUE | MATCH_QUEUE_ONLY,
  MULT_QUEUE        = CALL_QUEUE | MULT_QUEUE_ONLY,

  NO_CALL_QUEUE     = CALL_QUEUE,
  NO_MATCH_QUEUE    = MATCH_QUEUE_ONLY,
  NO_MULT_QUEUE     = MULT_QUEUE_ONLY,
                
  CALL_ACCT         = (1 << 3),
  MATCH_ACCT_ONLY   = (1 << 4),
  MULT_ACCT_ONLY    = (1 << 5),
  MATCH_ACCT        = CALL_ACCT | MATCH_ACCT_ONLY,
  MULT_ACCT         = CALL_ACCT | MULT_ACCT_ONLY,

  NO_CALL_ACCT      = CALL_ACCT,
  NO_MATCH_ACCT     = MATCH_ACCT_ONLY,
  NO_MULT_ACCT      = MULT_ACCT_ONLY,
                    
  CALL_MSG          = (1 << 6),
  MATCH_MSG_ONLY    = (1 << 7),
  MULT_MSG_ONLY     = (1 << 8),
  MATCH_MSG         = CALL_MSG | MATCH_MSG_ONLY,
  MULT_MSG          = CALL_MSG | MULT_MSG_ONLY,

  NO_CALL_MSG       = CALL_MSG,
  NO_MATCH_MSG      = MATCH_MSG_ONLY,
  NO_MULT_MSG       = MULT_MSG_ONLY,
                    
  LOG_ERROR         = (1 << 9),
  NO_LOG_ERROR      = LOG_ERROR,

  EXPECT_ALL        = ~EXPECT_NONE
} expectation_t;

typedef enum {
  VERBOSE_DEBUG,
  VERBOSE_WARN,
  VERBOSE_ERROR,
  VERBOSE_NONE,
} verbose_t;

struct client_st {
  int message_callback_called;
  int queue_callback_called;
  int account_callback_called;
  int complete_callback_called;
  int custom_malloc_called;
  int custom_free_called;
  
  expectation_t must, must_not;
  expectation_t result;
  
  verbose_t verbose;
  
  burrow_st *burrow;
  
  const char *acct;
  const char *queue;
  const char *msgid;
  size_t body_size;
  void *body;
};
 
typedef struct client_st client_st;

client_st *test_setup(const char *backend);
void test_teardown(client_st *client);
void test_run_functional(client_st *client);

#endif /* __BURROW_GENERIC_TESTS_H */