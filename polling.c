#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>

typedef struct struct_booth
{
  int num;
  int voters;
  int EVM;
  int capacity;
}struct_booth;

typedef struct struct_EVM
{
  int num;
  int booth_num;
  int capacity;
  int cur_cap;
  int cur_voters;
}struct_EVM;

typedef struct struct_VOTER
{
  int num;
  int booth_num;
  int visited;
}struct_VOTER;

typedef struct pair
{
  int F;
  int S;
}pair;

struct_booth *booth;
struct_EVM **EVM;
struct_VOTER **VOTER;
int *flag;
int *evm_flag;

pthread_mutex_t *mutex;
pthread_cond_t *thread_flag;
pthread_cond_t *evm_thread_flag;
pthread_t *booth_tid;
pthread_t **voter_tid;
pthread_t **evm_tid;
pthread_attr_t attr;
pair **args_evm;
pair **args_voter;

void voter_in_slot(pair arg_voter)
{
  int i,j;
  i = arg_voter.F;
  j = arg_voter.S;
  //pthread_mutex_lock(&mutex[voter.booth_num]);
  printf("Voter %d at Booth %d got allocated EVM %d\n",VOTER[i][j].num,VOTER[i][j].booth_num,flag[VOTER[i][j].booth_num]);
  VOTER[i][j].visited = 1;
  EVM[VOTER[i][j].booth_num][flag[VOTER[i][j].booth_num]].cur_voters++;
  flag[VOTER[i][j].booth_num] = 0;
  booth[VOTER[i][j].booth_num].voters--;
  pthread_cond_signal(&evm_thread_flag[VOTER[i][j].booth_num]);
  //pthread_mutex_unlock(&mutex[voter.booth_num]);
}

void voter_wait_for_evm(pair arg_voter)
{
  int i,j;
  i = arg_voter.F;
  j = arg_voter.S;
  //printf("VOTER:%d BOOTH:%d\n",voter.num,voter.booth_num);
  pthread_mutex_lock(&mutex[VOTER[i][j].booth_num]);
  //printf("VOTER:::%d BOOTH:::%d :::%d\n",voter.num,voter.booth_num,booth[voter.booth_num].voters);
  while(!flag[VOTER[i][j].booth_num])
  {
    pthread_cond_wait(&thread_flag[VOTER[i][j].booth_num],&mutex[VOTER[i][j].booth_num]);
  }
  //printf("VOTER::::::::::%d BOOTH::::::::::::%d ::::::%d\n",voter.num,voter.booth_num,booth[voter.booth_num].voters);
  voter_in_slot(arg_voter);
  pthread_mutex_unlock(&mutex[VOTER[i][j].booth_num]);
  //voter_in_slot(voter);
}

void *polling_ready_evm(pair arg_evm)
{
  int i,j;
  i = arg_evm.F;
  j = arg_evm.S;
  //printf("BOOTH:%d EVM:%d VOTERS:%d\n",evm.booth_num,evm.num,booth[evm.booth_num].voters);
  if(booth[EVM[i][j].booth_num].voters <= 0)
  {
    //printf("BOOTH:%d EVM:%d\n",EVM[i][j].booth_num,EVM[i][j].num);
    return NULL;
  }
  printf("EVM %d at Booth %d is free with slots = %d\n",EVM[i][j].num,EVM[i][j].booth_num,EVM[i][j].cur_cap);
  while(EVM[i][j].cur_voters < EVM[i][j].cur_cap)
  {
    //printf("BOOTH::%d EVM:%d VOTERS:%d CAP:%d\n",EVM[i][j].booth_num,EVM[i][j].num,EVM[i][j].cur_voters,EVM[i][j].cur_cap);
    if(booth[EVM[i][j].booth_num].voters <= 0)
    {
      if(EVM[i][j].cur_voters>0)
      {
        printf("EVM %d at Booth %d is moving for voting phase\n",EVM[i][j].num,EVM[i][j].booth_num);
        printf("EVM %d at Booth %d has finished Voting phase\n",EVM[i][j].num,EVM[i][j].booth_num);
      }
      //printf("BOOTH:%d EVM:%d\n",EVM[i][j].booth_num,EVM[i][j].num);
      return NULL;
    }
    pthread_mutex_lock(&mutex[EVM[i][j].booth_num]);
    flag[EVM[i][j].booth_num] = EVM[i][j].num;
    pthread_cond_signal(&thread_flag[EVM[i][j].booth_num]);
    //EVM[i][j].cur_voters++;
    //booth[evm.booth_num].voters--;
    pthread_mutex_unlock(&mutex[EVM[i][j].booth_num]);
  }
  //printf("BOOTH:::%d EVM::::%d VOTERS:::%d\n",EVM[i][j].booth_num,EVM[i][j].num,booth[EVM[i][j].booth_num].voters);

  pthread_mutex_lock(&mutex[EVM[i][j].booth_num]);
  while((EVM[i][j].cur_cap-EVM[i][j].cur_voters) != 0)
  {
    pthread_cond_wait(&evm_thread_flag[EVM[i][j].booth_num],&mutex[EVM[i][j].booth_num]);
  }
  pthread_mutex_unlock(&mutex[EVM[i][j].booth_num]);
  printf("EVM %d at Booth %d is moving for voting phase\n",EVM[i][j].num,EVM[i][j].booth_num);
  printf("EVM %d at Booth %d has finished Voting phase\n",EVM[i][j].num,EVM[i][j].booth_num);
}

void *create_VOTER(void *arg)
{
  int i,j;
  pair arg_voter;
  arg_voter = *((pair *)arg);
  i = arg_voter.F;
  j = arg_voter.S;
  voter_wait_for_evm(arg_voter);
  //printf("BOOTH:%d VOTER:%d\n",VOTER[i][j].booth_num,VOTER[i][j].num);
  return NULL;
}

void *create_EVM(void *arg)
{
  int i,j;
  pair arg_evm = *((pair *)arg);
  i = arg_evm.F;
  j = arg_evm.S;
  while(1)
  {
    EVM[i][j].cur_cap = rand()%EVM[i][j].capacity+1;
    EVM[i][j].cur_voters = 0;
    if(booth[EVM[i][j].booth_num].voters <= 0)
    {
      //printf("BOOTH:%d EVM:%d\n",EVM[i][j].booth_num,EVM[i][j].num);
      return NULL;
    }
    polling_ready_evm(arg_evm);
    if(booth[EVM[i][j].booth_num].voters <= 0)
    {
      //printf("BOOTH:%d EVM:%d\n",EVM[i][j].booth_num,EVM[i][j].num);
      return NULL;
    }
  }
}

void *create_booth(void *args)
{
  int i = *((int *)args);
  for(int k=1;k<=booth[i].voters;k++)
  {
    VOTER[i][k].num = k;
    VOTER[i][k].booth_num = booth[i].num;
    VOTER[i][k].visited = 0;
    args_voter[i][k].F = i;
    args_voter[i][k].S = k;
    pthread_create(&voter_tid[i][k],&attr,create_VOTER,&args_voter[i][k]);
  }
  for(int j=1;j<=booth[i].EVM;j++)
  {
    EVM[i][j].num = j;
    EVM[i][j].booth_num = booth[i].num;
    EVM[i][j].capacity = booth[i].capacity;
    EVM[i][j].cur_cap = 0;
    EVM[i][j].cur_voters = 0;
    args_evm[i][j].F = i;
    args_evm[i][j].S = j;
    pthread_create(&evm_tid[i][j],&attr,create_EVM,&args_evm[i][j]);
  }
  for(int j=1; j<=booth[i].EVM; j++)
    pthread_join(evm_tid[i][j], 0);
  for(int j=1; j<=booth[i].voters; j++)
    pthread_join(voter_tid[i][j], 0);
  printf("BOOTH %d has FINISHED VOTING\n",booth[i].num);
  return NULL;
}

int main(int argc,char *argv[])
{
  int n,i,j,k;
  int *arg;
  scanf("%d",&n);
  booth = (struct_booth *)malloc((n+5)*sizeof(struct_booth));
  EVM = (struct_EVM **)malloc((n+5)*sizeof(struct_EVM *));
  VOTER = (struct_VOTER **)malloc((n+5)*sizeof(struct_VOTER *));
  args_evm = (pair **)malloc((n+5)*sizeof(pair));
  args_voter = (pair **)malloc((n+5)*sizeof(pair));
  mutex = (pthread_mutex_t *)malloc((n+5)*sizeof(pthread_mutex_t));
  thread_flag = (pthread_cond_t *)malloc((n+5)*sizeof(pthread_cond_t));
  evm_thread_flag = (pthread_cond_t *)malloc((n+5)*sizeof(pthread_cond_t));
  flag = (int *)malloc((n+5)*sizeof(int));
  evm_flag = (int *)malloc((n+5)*sizeof(int));
  arg = (int *)malloc((n+5)*sizeof(int));
  evm_tid = (pthread_t **)malloc((n+5)*sizeof(pthread_t *));
  voter_tid = (pthread_t **)malloc((n+5)*sizeof(pthread_t *));
  booth_tid = (pthread_t *)malloc((n+5)*sizeof(pthread_t));
  for(i=1;i<=n;i++)
  {
    booth[i].num = i;
    scanf("%d %d %d",&booth[i].voters,&booth[i].EVM,&booth[i].capacity);
    EVM[i] = (struct_EVM *)malloc((booth[i].EVM+5)*sizeof(struct_EVM));
    VOTER[i] = (struct_VOTER *)malloc((booth[i].voters+5)*sizeof(struct_VOTER));
    evm_tid[i] = (pthread_t *)malloc((booth[i].EVM+5)*sizeof(pthread_t));
    voter_tid[i] = (pthread_t *)malloc((booth[i].voters+5)*sizeof(pthread_t));
    args_evm[i] = (pair *)malloc((booth[i].EVM+5)*sizeof(pair));
    args_voter[i] = (pair *)malloc((booth[i].voters+5)*sizeof(pair));
  }
  for(i=1;i<=n;i++)
  {
    pthread_mutex_init(&mutex[i],NULL);
    pthread_cond_init(&thread_flag[i],NULL);
    pthread_cond_init(&evm_thread_flag[i],NULL);
    flag[i] = 0;
    evm_flag[i] = 0;
  }
  pthread_attr_init(&attr);
  for(i=1;i<=n;i++)
  {
    arg[i] = i;
    pthread_create(&booth_tid[i],&attr,create_booth,&arg[i]);
  }
  for(i=1;i<=n;i++)
    pthread_join(booth_tid[i], 0);
  return 0;
}
