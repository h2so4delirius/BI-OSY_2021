#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <string>
#include <vector>
#include <array>
#include <iterator>
#include <set>
#include <list>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <condition_variable>
#include <pthread.h>
#include <semaphore.h>
#include "progtest_solver.h"
#include "sample_tester.h"
using namespace std;
#endif /* __PROGTEST__ */ 

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

class CQualityControl
{
  public:
    void ready_asheet(int id){
      int count_r=0;
    
      while (1)
      {
            
              std::unique_lock<std::mutex> lk(*mt_chseet[id]);
              cv2[id]->wait(lk,[this,id]{
                //if(end_work){return true;}
                if(have_change[id]||end_workers==workThreads){return true;}
                return ready_work[id].size()!=0;
                });
          if(end_workers==workThreads&&!ready_work[id].size()){break;};
            if(ready_work[id].find(count_r)!=ready_work[id].end()){
              line[id]->doneSheet(ready_work[id].at(count_r));
              ready_work[id].erase(count_r);
              count_r++;
            }
            else{have_change[id] = false;}
          
          lk.unlock();
          if(have_change[id]){cv2[id]->notify_all();}
      }
      return;
    }





    void worker(){
      while (1)
      {
          int id_roll=0;
          int id_asheet=0;
          ASheet pop;
              std::unique_lock<std::mutex> lk(mt_front);
              cv1.wait(lk,[this]{
                if(end_work){return true;}
                return acheet_to_work.size()!=0;
                });
            if(end_work&&acheet_to_work.empty()){ lk.unlock();/*cout<<" work end\n";*/ break;}
            pair<ASheet,pair<int,int>> a = acheet_to_work.front();
            acheet_to_work.pop();
            lk.unlock();
            pop =  a.first;
            id_roll = a.second.first;
            id_asheet = a.second.second;
            int **k=new int*[pop->m_Length];
            for(int i=0;i<pop->m_Length;i++){
              k[i]= new int[pop->m_Width];
              for(int j=0;j<pop->m_Width;j++){
                k[i][j]=pop->m_Thickness[pop->m_Width*i+j];
              }
            }
            
            for(auto &i:pop->m_RelDev){
              pop->updateRelDev(i.first,maxRectByRelDev(k,pop->m_Width,pop->m_Length,i.first));
            }
            for(auto &i:pop->m_Volume){
              pop->updateVolume(i.first,maxRectByVolume(k,pop->m_Width,pop->m_Length,i.first));
            }
            for(auto &i:pop->m_MinMax){
              pop->updateMinMax(i.first,maxRectByMinMax(k,pop->m_Width,pop->m_Length,i.first.m_Lo,i.first.m_Hi));
            }
            mt_chseet[id_roll]->lock();
            ready_work[id_roll][id_asheet]=pop;
            have_change[id_roll] = true;
            cv2[id_roll]->notify_one();
            mt_chseet[id_roll]->unlock();
            
          
      }
      mt.lock();
      end_workers+=1;
      mt.unlock();
      for(auto &i:cv2){
        i->notify_all();
      }
      return;
    }



    void get_chseet(int numb){
      int count=0;
      while(1){
        ASheet cop = line[numb]->getSheet();
        if(!cop){
          break;
        }
        mt_front.lock();
        acheet_to_work.push(make_pair(cop,make_pair(numb,count)));
        mt_front.unlock();
        cv1.notify_one();
        count++;
      }
      mt.lock();
      end_rolling+=1;
      if(end_rolling==line.size()){cv1.notify_all();end_work=true;}
      mt.unlock();
      
      return;
    }

    static void checkAlgorithm(ASheet pop){

            int **k=new int*[pop->m_Length];
            for(int i=0;i<pop->m_Length;i++){
              k[i]= new int[pop->m_Width];
              for(int j=0;j<pop->m_Width;j++){
                k[i][j]=pop->m_Thickness[pop->m_Width*i+j];
              }
            }
            for(auto &i:pop->m_RelDev){
              pop->updateRelDev(i.first,maxRectByRelDev(k,pop->m_Width,pop->m_Length,i.first));
            }
            for(auto &i:pop->m_Volume){
              pop->updateVolume(i.first,maxRectByVolume(k,pop->m_Width,pop->m_Length,i.first));
            }
            for(auto &i:pop->m_MinMax){
              pop->updateMinMax(i.first,maxRectByMinMax(k,pop->m_Width,pop->m_Length,i.first.m_Lo,i.first.m_Hi));
            }      
      
      
      return;
      }


    void addLine ( AProductionLine line )
    {
      this->line.push_back(line);
      return;
    }

    void start ( int workThreads ){
      this->workThreads=workThreads;
      int size = line.size();
      for(int i=0;i<size;i++){
        //how.push_back(0);
        have_change.push_back(false);
        mt_chseet.push_back(new mutex());
        cv2.push_back(new condition_variable());
        vector<pair<ASheet,int>>b;
        //ready_work.push_back(b);
        work1.emplace_back(thread(&CQualityControl::get_chseet,this,i));
        //work1[i].join();
      }
      for(int i=0;i<workThreads;i++){
         work2.emplace_back(thread(&CQualityControl::worker,this));
         //work2[i].join();
      }
      for(int i=0;i<size;i++){
         work3.emplace_back(thread(&CQualityControl::ready_asheet,this,i));
      }



      return;
    }
    void stop( void ){
      for(unsigned int i=0;i<line.size();i++){
        if(work3[i].joinable())
        work3[i].join();
        work1[i].join();
      }
      for(int i=0;i<workThreads;i++){
        work2[i].join();
      }
      return;
    }
  private:
  
  mutex mt;
  mutex mt_front;
  vector<mutex*> mt_chseet;
  condition_variable cv1;
  vector<condition_variable*> cv2;
  vector<AProductionLine>line;
  vector<thread>work1;
  vector<thread>work2;
  vector<thread>work3;
  vector<int>how;
  vector<bool> have_change;
  int num_workers_free=0;
  //vector<vector<pair<ASheet,int>>>ready_work;
  map<int,map<int,ASheet>>ready_work;
  queue<pair<ASheet,pair<int,int>>>acheet_to_work;
  //vector<pair<ASheet,pair<int,int>>>acheet_to_work;
  int end_workers = 0;
  long unsigned int end_rolling=0;
  bool end_work=false;
  bool end=false;
  int num_work=0;
  int workThreads=0;
};

// TODO: CQualityControl implementation goes here
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__
int                main                                    ( void )
{
  
  CQualityControl control;
  AProductionLineTest line = std::make_shared<CProductionLineTest> ();
  control . addLine ( line );
  control . start ( 1 );
  control . stop  ();
  if ( ! line -> allProcessed () )
    throw std::logic_error ( "(some) sheets were not correctly processsed" );
  return 0;  
}
#endif /* __PROGTEST__ */ 
