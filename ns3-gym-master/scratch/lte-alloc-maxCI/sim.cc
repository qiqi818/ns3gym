/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Piotr Gawlowicz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/opengym-module.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include <ns3/epc-mme.h>
#include <ns3/object.h>
#include <ns3/spectrum-interference.h>
#include <ns3/spectrum-error-model.h>
#include <ns3/log.h>
#include <ns3/packet.h>
#include <ns3/ptr.h>

#include "ns3/radio-bearer-stats-calculator.h"
#include <ns3/constant-position-mobility-model.h>
#include <ns3/eps-bearer.h>
#include <ns3/node-container.h>
#include <ns3/mobility-helper.h>
#include <ns3/net-device-container.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/lte-enb-net-device.h>
#include <ns3/lte-ue-rrc.h>
#include <ns3/lte-helper.h>
#include "ns3/string.h"
#include "ns3/double.h"
#include <ns3/lte-enb-phy.h>
#include <ns3/lte-ue-phy.h>
#include <ns3/boolean.h>
#include <ns3/enum.h>
#include <ns3/string.h>
#include <ns3/epc-helper.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include <ns3/object-factory.h>
//Add for input-defaults.txt file
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include <iostream>
//FlowMonitor
#include "ns3/flow-monitor-module.h"
using namespace ns3;
using namespace std;
/*
  根据带宽确定一个rbg包含prb的数量
*/ 
uint32_t getRbgSizeFromBandwidth(uint32_t bandwidth)
{
  vector<uint32_t> band = {6,12,25,50,75,100};//1.25MHz, 3.0MHz, 5.0MHz, 10.0MHz, 15.0MHz, 20.0MHz
  vector<uint32_t> size = {1,2,2,3,4,4};
  for(uint32_t i = 0 ; i < band.size(); i++)
  {
    if(band[i] == bandwidth)
      return size[i]; //查表
  }
  return 0;
}
vector<double> delay_buget = {100,150,50,300,100,300,100,300,300};//时延预算，对应不同的qci属性
float stepCounter = -1; //step计数


list<double> transRight;
list<double> allToTrans;
double remainToTrans = 0.0;
struct rParameters {
  //map的key都为rnti
  uint32_t cellid;
  map<uint32_t, uint32_t> peruerbbitmap;  //调度情况
  map<uint32_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters> rlcbuffer;//rlc层初始情况   队首时延和待传数据量
  map<uint32_t, vector<double>> phyrxstates;//物理层接收状况统计
  map<uint32_t, double> rlcrxstate;//调度完成后rlc层情况   队首时延和剩余待传数据量
  map<uint32_t, vector<uint32_t>> uestates;  //ue的相关信息  现有cqi和qci
  map<uint32_t, SpectrumValue> sinr;   //信干噪比
};

list< vector< struct rParameters > > rewardParameters;//用来存储奖励相关参数的变量，时间<小区<struct>>
uint32_t  bandwidth = 100;
uint32_t rbgSize = getRbgSizeFromBandwidth(bandwidth);//获取rbgsize，即一个rbg包含多少个prb


uint16_t numberOfenb = 3;//enb数目
uint16_t numberOfRandomUes = 500;//ue数量
uint16_t numOfrbg = bandwidth/rbgSize;//rbg数量

Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
FlowMonitorHelper flowmon;
Ptr<FlowMonitor> monitor;

NetDeviceContainer enbDevs;
NetDeviceContainer ueDevs;
bool isFirstWrite = true;
NS_LOG_COMPONENT_DEFINE ("OpenGym");


/*
split函数，分割字符串
 */
std::vector<std::string>
split (const char *s, const char *delim)
{
  std::vector<std::string> result;
  if (s && strlen (s))
    {
      int len = strlen (s);
      char *src = new char[len + 1];
      strcpy (src, s);
      src[len] = '\0';
      char *tokenptr = strtok (src, delim);
      while (tokenptr != NULL)
        {
          std::string tk = tokenptr;
          result.emplace_back (tk);
          tokenptr = strtok (NULL, delim);
        }
      delete[] src;
    }
  return result;
}
/*
  获取物理层接收状态的回调函数
*/
void DlPhyReceptionCallback(std::string path, PhyReceptionStatParameters params)
{
  map<uint32_t, vector<double>>::iterator it = rewardParameters.back()[params.m_cellId-1].phyrxstates.find(params.m_rnti);
  if(it == rewardParameters.back()[params.m_cellId-1].phyrxstates.end() || (it != rewardParameters.back()[params.m_cellId-1].phyrxstates.end() && (int64_t)it->second[1] != params.m_timestamp))
  {
    
    if(it == rewardParameters.back()[params.m_cellId-1].phyrxstates.end())
    {
          // 0 第一次发传输时间
          // 1 最后一次传输时间
          // 2 总传送块数
          // 3 正确传输的块数
          // 4 总传输量
          // 5 传对的量
          // 6 最近一次发包是否正确
      vector<double> temp(7,0);
      temp[0] = params.m_timestamp;
      temp[1] = params.m_timestamp;
      temp[2] = 1;
      temp[4] = params.m_size;
      if(params.m_correctness == 1)
      {
        temp[3] = 1;
        temp[5] = params.m_size;
        temp[6] = 1;
      }
      else
      {
        temp[3] = 0;
        temp[5] = 0;
        temp[6] = 0;
      }
      rewardParameters.back()[params.m_cellId-1].phyrxstates.insert(pair<uint32_t, vector<double>>(params.m_rnti, temp));
    }
    else
    {
      it->second[1] = params.m_timestamp;
      it->second[2] += 1;
      it->second[4] += params.m_size;
      if(params.m_correctness == 1)
      {
        it->second[3] += 1;
        it->second[5] += params.m_size;
        it->second[6] = 1;
      }
      else
      {
        it->second[3] += 0;
        it->second[5] += 0;
        it->second[6] = 0;
      }
    }
  }


  
}

void tracephyrx()
{
  Config::Connect("/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/LteUePhy/DlSpectrumPhy/DlPhyReception",
                   MakeCallback(&DlPhyReceptionCallback));
}

Ptr<LteEnbNetDevice>
GetEnb (int enb_idx)
{
  return enbDevs.Get (enb_idx)->GetObject<LteEnbNetDevice> ();
}

Ptr<CqaFfMacScheduler>
GetSchedulerForEnb (int enb_idx)
{
  PointerValue ptr;
  GetEnb (enb_idx)->GetCcMap ().at (0)->GetAttribute ("FfMacScheduler", ptr);
  Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler> ();
  Ptr<CqaFfMacScheduler> pff = ff->GetObject<CqaFfMacScheduler> ();
  return pff;
}
/*
Define observation space
用于gym从ns3获取状态空间
未使用此函数
*/
Ptr<OpenGymSpace>
MyGetObservationSpace (void)
{
  cout << "-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+" << endl;
  
  uint32_t nodeNum = 5;
  float low = 0.0;
  float high = 10.0;
  std::vector<uint32_t> shape = {
      nodeNum,
  };
  std::string dtype = TypeNameGet<uint32_t> ();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);
  
  return space;
}

/*
Define action space
用于gym从ns3获取动作空间
未使用此函数
*/
Ptr<OpenGymSpace>
MyGetActionSpace (void)
{
  
  Ptr<OpenGymTupleSpace> space = CreateObject<OpenGymTupleSpace> ();
  for (uint16_t i = 0; i < ueDevs.GetN()*numOfrbg*3; i++)
    space->Add (CreateObject<OpenGymDiscreteSpace> (1));
  return space;
}

/*
Define game over condition
用于传递仿真结束的标志
*/
bool
MyGetGameOver (void)
{
  bool isGameOver = false;
  NS_LOG_UNCOND ("GameOver: " << isGameOver);
  return isGameOver;
}


/*
Collect observations
用于从ns3中统计环境信息，封装到OpenGymBoxContainer中
*/
Ptr<OpenGymDataContainer>
MyGetObservation(void)
{
  stepCounter += 1;
  cout << "----------------------以上为step: " << stepCounter-1 << "时刻----------------------" << endl;
  cout << endl;
  cout << "----------------------以下为step: " << stepCounter << "时刻----------------------" << endl;
  cout << "OBS for step: " << stepCounter << " 调度前的RlcBuffer" << endl;
  uint32_t nodeNum = enbDevs.GetN ();
  std::vector<uint32_t> shape = {
      nodeNum,
  };

  Ptr<OpenGymBoxContainer<uint32_t>> box = 
  CreateObject<OpenGymBoxContainer<uint32_t>>(shape); //创建OpenGymBoxContainer用以包装存储observation，进而传递给gym

  list<list<int>> ls;
  cout << "eNB\tRNTI\tLCID\t时延tx\t时延retx\tTXLen\tReTxLen" << endl;
  cout << "===============================================================" << endl;
  for (uint16_t i = 0; i < enbDevs.GetN(); i++)
  {
    Ptr<LteEnbRrc> enbRrc = GetEnb(i)->GetRrc ();
    Ptr<CqaFfMacScheduler> pff = GetSchedulerForEnb(i);

    // for (std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
    //          it = pff->m_rlcBufferReq.begin ();
    //      it != pff->m_rlcBufferReq.end (); it++)
    for (auto it = pff->m_rlcBufferReq.begin (); it != pff->m_rlcBufferReq.end (); it++)
    {
      ns3::EpsBearer::Qci qci = ns3::EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
      uint16_t rnti = it->first.m_rnti;
      FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& rrp = it->second;
      if (rnti != 0 && rrp.m_rlcTransmissionQueueSize != 0)
      {
        cout << (i + 1) << "\t" << rnti << "\t" << (uint16_t) (it->first.m_lcId) << "\t";
        cout << rrp.m_rlcTransmissionQueueHolDelay << "\t" << rrp.m_rlcRetransmissionHolDelay << "\t";
        cout << rrp.m_rlcTransmissionQueueSize << "\t";
        cout << rrp.m_rlcRetransmissionQueueSize << endl;
      }
      if (enbRrc->HasUeManager(rnti))
      {
        std::map<uint8_t, Ptr<LteDataRadioBearerInfo>>::iterator 
        pdrb = enbRrc->GetUeManager(rnti)->m_drbMap.begin();
        qci = (++pdrb)->second->m_epsBearer.qci;
      }
      Ptr<LteAmc> amc = pff->m_amc;
      uint8_t cqi = pff->m_p10CqiRxed.find (rnti)->second;
      uint8_t worstCqi = 15;
      
      try{
        for(uint32_t o = 0 ; o < numOfrbg; o++)
        {
          if(pff->m_a30CqiRxed.find (rnti) != pff->m_a30CqiRxed.end())
          {
            int8_t tempCqi = (uint32_t)pff->m_a30CqiRxed.find (rnti)->second.m_higherLayerSelected.at(o).m_sbCqi.at(0);
            if(worstCqi > (uint32_t)tempCqi)
              worstCqi = tempCqi;
            // cout << (uint32_t)pff->m_a30CqiRxed.find (rnti)->second.m_higherLayerSelected.at(o).m_sbCqi.at(0) << endl;
          }
        }
      }
      catch(std::out_of_range)
      {
        
      }
      // cout << "----: " << (uint32_t) worstCqi << endl;
      if(worstCqi < cqi)
        cqi = worstCqi;
      // cout << "====: " << (uint32_t) cqi << endl;
      uint8_t mcs = amc->GetMcsFromCqi (cqi);
      // cout << "-------mcs: " << (uint32_t)mcs << endl;
      uint32_t nOfprb = 0;
      if (rnti != 0 && rrp.m_rlcTransmissionQueueSize != 0)
      {
        nOfprb = rbgSize;
        //统计每个ue需要多少个prb
        while ((uint32_t) (double(amc->GetDlTbSizeFromMcs (mcs, nOfprb)) / 8) <
                    rrp.m_rlcTransmissionQueueSize && nOfprb < bandwidth/rbgSize)
        {
          nOfprb += rbgSize;
        }
      }

      //拆分状态
      for (uint32_t k = 0; k < nOfprb / rbgSize; k++)
      {
        list<int> temp;
        if (it->first.m_rnti != 0 && it->second.m_rlcTransmissionQueueSize != 0)
        {
          temp.push_back(it->second.m_rlcTransmissionQueueSize);


          temp.push_back (i);
          temp.push_back (rnti);
          temp.push_back (cqi);
          temp.push_back (qci);
        }
        ls.push_back (temp);
        }

    }
    ls.sort (); //依等待传输的数据量排序
    ls.reverse();

  }

  uint16_t num_ue = 0; //统计总的业务请求数
  vector<uint32_t> unumpercell(enbDevs.GetN(),0);
  //将ls中的状态存入box，若请求数大于enbDevs.GetN()*numOfrbg，则截断
  for (list<list<int>>::iterator it = ls.begin (); it != ls.end () && num_ue < enbDevs.GetN()*numOfrbg; it++)
    {
      uint32_t cell = *(++(it->begin()));
      if(unumpercell[cell] < numOfrbg)
      {
        for (list<int>::iterator iz = it->begin (); iz != it->end (); iz++)
          {
            if (iz == it->begin ())
            {
              // iz++;
              continue;
            }
            
            box->AddValue (*(iz)); //将请求存在OpenGymBoxContainer中等待传送
          }
          box->AddValue (0);//
        num_ue++;
        unumpercell[cell]++;
      }
    }
    Ptr<OpenGymBoxContainer<uint32_t>> box1 = CreateObject<OpenGymBoxContainer<uint32_t>>(shape); //创建OpenGymBoxContainer用以包装存储observation，进而传递给gym


  //进行补0
  
    for (uint16_t i = 0; i < enbDevs.GetN()*numOfrbg; i++)
      {
        box1->AddValue (0);
        box1->AddValue (0);
        box1->AddValue (0);
        box1->AddValue (0);
        box1->AddValue (0);
      }
  
  

  return box1;
}

/*
Define extra info. Optional
用于传递其他信息，暂未使用
*/
std::string
MyGetExtraInfo (void)
{
  std::string myInfo = "testInfo";
  myInfo += "|123";
  NS_LOG_UNCOND ("ExtraInfo: " << myInfo);
  return myInfo;
}

/*
Execute received actions
ns3接收到从gym中传递过来的封装在OpenGymDataContainer中的动作action，并将其以适当的形式作为参数传递到调度器中执行
*/
bool
MyExecuteActions (Ptr<OpenGymDataContainer> action)
{
  Ptr<OpenGymTupleContainer> tu_ac = DynamicCast<OpenGymTupleContainer>(action); 
  //将传递过来的action自动转换为OpenGymTupleContainer
  //传递过来的OpenGymTupleContainer封装的动作的形式是(CellID1,RNTI1,资源编号1,CellID2,RNTI2,资源编号2,CellID3,RNTI3,资源编号3,……)
  //从OpenGymTupleContainer中读取数据并保存到字符串中
  string ac_s[enbDevs.GetN()];
  for(uint32_t j = 0 ; j < enbDevs.GetN(); j++)
  {
    ac_s[j] = "";
  }
  for (uint32_t i = 0; i < tu_ac->size(); i += 3)
    {
      //如果没有动作，标记为9999
      if (DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 2))->GetValue () == 9999)
        {
          for(uint32_t k = 0 ; k < enbDevs.GetN(); k++)
          {
            ac_s[k] += to_string (9999);
          }
        }
      else
        {
          //第i个波束（eNB）的调度信息保存为 RNTI1 资源编号1 RNTI2 资源编号2 RNTI3 资源编号3 …… 形式的字符串
          uint32_t enb =  DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i))->GetValue ();//获取eNB序号
          uint32_t rnti = DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 1))->GetValue ();//rnti
          uint32_t rbg = DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 2))->GetValue ();//资源编号

          ac_s[enb] += (to_string (rnti) + " " + to_string (rbg));
          ac_s[enb] += " ";
        }
    }
  cout << endl;
  NS_LOG_UNCOND ("(" << stepCounter << "时刻)Action: \n" << tu_ac);
  //以字符串的形式将动作传递到各个波束（eNB）进行调度
  cout << endl;
  cout << "(" << stepCounter << "时刻)调度结果: " << endl;
  // for (uint16_t i = 0; i < enbDevs.GetN(); i++)
  //   {
      
  //     PointerValue ptr;
  //     enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetCcMap ().at (0)->GetAttribute (
  //         "FfMacScheduler", ptr); //获取波束(eNB)i的调度器
  //     Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler> ();
  //     StringValue c;
  //     //通过SetAttribute将动作参数传到调度器中
  //     ff->SetAttribute ("test", StringValue (ac_s[i]));
  //     ff->SetAttribute ("cellid", StringValue (to_string(i)));
  //     //分别打印各个波束的调度信息
  //     ff->GetAttribute ("test", c);
  //     vector<string> vcs = split (c.Get ().c_str (), " ");
  //   }
   
  return true;
}

/*
Define reward function
用于gym从ns3中获取动作执行后的反馈信息
*/
float
MyGetReward (void)
{
  /* 
    计算 T时刻 的reward所需的参数获取时间
    rlcbuffer                  T                调度器
    rlcrxstate                 T                调度器 使用schedule函数进行事件调度
    prbbitmap(调度结果)         T+1               调度器
    phyrxstate                 T+2              trace
    sinr                       T+2              使用schedule函数进行事件调度
  */
  //用trace获取物理层接收状态
  tracephyrx();
  vector< struct rParameters > rv(enbDevs.GetN());
 
  ofstream dataFile2;
  ofstream dataFile1;
  if(isFirstWrite)
  {
    dataFile1.open("throughoutput.txt", ofstream::trunc);
    dataFile2.open("throughoutputrate.txt",  ofstream::trunc);
    dataFile1.close();
    dataFile2.close();
    isFirstWrite = false;
  }
  dataFile1.open("./scratch/lte-alloc-maxCI/throughoutput.txt", ofstream::app);//吞吐量
  dataFile2.open("./scratch/lte-alloc-maxCI/throughoutputrate.txt", ofstream::app);//吞吐率


  
  for(uint8_t i = 0; i < enbDevs.GetN(); i++)
  {
      Ptr<CqaFfMacScheduler> pff = GetSchedulerForEnb(i);
      Ptr<LteEnbRrc> enbRrc = GetEnb(i)->GetRrc ();
      rParameters r;
      r.cellid = i;
      map<uint32_t, uint32_t> bitmap;
      for(vector <struct BuildDataListElement_s>::iterator it = pff->m_ret.m_buildDataList.begin(); it != pff->m_ret.m_buildDataList.end();it++)
      {
        // cout << i+1 << endl;
        // cout << "rnti: " << it->m_dci.m_rnti << endl;
        // cout << "rbbit: " << it->m_dci.m_rbBitmap << endl;
        bitmap.insert(std::pair <uint32_t, uint32_t> (it->m_dci.m_rnti, it->m_dci.m_rbBitmap));
      }
      r.peruerbbitmap = bitmap;         //t时刻调度结果



      
      map<uint32_t, vector<uint32_t>> ue_states;
      for (std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
               it = pff->m_rlcBufferReq.begin ();
           it != pff->m_rlcBufferReq.end (); it++)
        {
          ns3::EpsBearer::Qci qci;
          if(enbRrc->HasUeManager(it->first.m_rnti))
        {
          std::map <uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator pdrb = enbRrc->GetUeManager(it->first.m_rnti)->m_drbMap.begin();
          qci = (++pdrb)->second->m_epsBearer.qci;
          
         }
          Ptr<LteAmc> amc = pff-> m_amc;
          uint8_t cqi = pff->m_p10CqiRxed.find(it->first.m_rnti)->second;
          vector<uint32_t> temp(2);
          temp[0] = cqi;
          temp[1] = (uint32_t)qci;
          ue_states.insert(std::pair <uint32_t, vector<uint32_t>>(it->first.m_rnti, temp)); 
        }
        r.uestates = ue_states;  //cqi和qci信息

        map<uint32_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters> rlcbuffer;
      for (std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
               it = pff->m_rlcBufferReq.begin ();
           it != pff->m_rlcBufferReq.end (); it++)
        {
          rlcbuffer.insert(pair<uint32_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>(it->first.m_rnti, it->second));
        }
        r.rlcbuffer = rlcbuffer;  //RLCBUFFER信息





        rv[i] = r;
        
  }
  

  rewardParameters.push_back(rv);
  //若此TTi没有接收数据，则不会触发tracephyrx，保存上一个TTi的phyrxstates
  if(rewardParameters.size() > 1)
  {
    for(uint32_t i = 0; i < enbDevs.GetN(); i++)
    {
      rewardParameters.back()[i].phyrxstates = (*(--(--rewardParameters.end())))[i].phyrxstates;
    }
  }


  //用收集的参数计算奖励----------------
  double reward = 0.0;
  vector< struct rParameters > rps0;
  vector< struct rParameters > rps1;
  vector< struct rParameters > rps2;
  vector< struct rParameters > rps3;
  double reward1 = 0.0; //delay
  double reward2 = 0.0; //信干噪比
  double reward3 = 0.0; //负奖励
  double reward4 = 0.0; //吞吐率
  // double unum = 0;
  // double sum = 0.0;
  // double count = 0.0;
  double dacnum = 0;//统计分配到资源的用户数
  if(rewardParameters.size() == 4)
  {
    rps0 = *(rewardParameters.begin());//T
    rps1 = *(++rewardParameters.begin());//T+1
    rps2 = *(++(++rewardParameters.begin()));//T+2
    rps3 = rewardParameters.back();//T+3


    double tx_right = 0;//正确传输量
    double tx_total = 0;//全部待传量
    double tx = 0.0;//全部传输量（包含错传）
    
    for(uint32_t i = 0 ; i < enbDevs.GetN(); i++)
    {
      // map<uint32_t, ns3::FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
      // for(it = rps1[i].rlcbuffer.begin(); it != rps1[i].rlcbuffer.end(); it++)
      // {
      //   if(rps1[i].uestates.find(it->first) != rps1[i].uestates.end())
      //   {
      //     uint32_t qci = rps1[i].uestates.find(it->first)->second[1];
      //     reward1 += 1.0 - (double)it->second.m_rlcTransmissionQueueHolDelay/delay_buget[qci-1];
      //     unum += 1;
      //   }
      // }


      // map<uint32_t, ns3::SpectrumValue>::iterator it1;
      // for(it1 = rps2[i].sinr.begin();it1 != rps2[i].sinr.end(); it1++)
      // {
        
      //   for(uint32_t i = 0 ; i < 25; i++)
      //   {
      //     if(it1->second[i] > 0)
      //     {
      //       sum += it1->second[i];
      //       count += 1;
      //     }
      //   }
        
      // }
      
       
       
       
      // map<uint32_t, ns3::FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it2;
      // for(it2 = rps0[i].rlcbuffer.begin(); it2 != rps0[i].rlcbuffer.end(); it2++)
      // {
        
      //   if(rps1[i].peruerbbitmap.find(it2->first) == rps1[i].peruerbbitmap.end() && (it2->second.m_rlcTransmissionQueueSize > 0 || it2->second.m_rlcRetransmissionQueueSize > 0))
      //   {
         
      //     uint32_t cqi = rps0[i].uestates.find(it2->first)->second[0];
      //     reward3 -= (double)cqi/15;
      //   }
      // }


      map<uint32_t, SpectrumValue>::iterator itsinr;
      for(itsinr = rps2[i].sinr.begin(); itsinr != rps2[i].sinr.end(); itsinr++)
      {
        double ok = 0;
        double count = 0;
        for(uint32_t m = 0; m < numOfrbg; m++)
        {
          if(itsinr->second[2*m] > 0)
          {
            count += 1;
          }
          if(itsinr->second[2*m] >= 50)
          {
            ok += 1;
          }
        }
        if(count > 0)
        {
          dacnum += 1;
          reward2 += ok/count;
        }
      }






      map<uint32_t, ns3::FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it3;

      for(it3 = rps0[i].rlcbuffer.begin(); it3 != rps0[i].rlcbuffer.end(); it3++)
      {

        tx_total += it3->second.m_rlcTransmissionQueueSize;

        if(rps2[i].phyrxstates.find(it3->first) != rps2[i].phyrxstates.end())
        {
          if((uint32_t)rps2[i].phyrxstates.find(it3->first)->second[6] == 1 && (int64_t)rps2[i].phyrxstates.find(it3->first)->second[1] == Simulator::Now().GetMilliSeconds()-1)
          {

            tx_right += (rps0[i].rlcrxstate.find(it3->first)->second);
            
          }
          if((int64_t)rps2[i].phyrxstates.find(it3->first)->second[1] == Simulator::Now().GetMilliSeconds()-1)
          {

            tx += (rps0[i].rlcrxstate.find(it3->first)->second);
            
          }
          
        }

      }


      remainToTrans = 0.0;
      // map<uint32_t, ns3::FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it3;

      for(auto it4 = rps0[i].rlcrxstate.begin(); it4 != rps0[i].rlcrxstate.end(); it4++)
      {
        remainToTrans += it4->second;
      }

    }

    


    if(tx_right > 1000000)
        tx_right = 0;
    transRight.push_back(tx_right);
    allToTrans.push_back(tx_total);
    

    if(transRight.size() == 300)
    {
      double r = 0.0;
      double t = 0.0;

      for(auto b = transRight.begin() ; b != transRight.end(); b++)
      {
        r += *b;
      } 
      transRight.pop_front();

      for(auto b = allToTrans.begin() ; b != allToTrans.end(); b++)
      {
        t += *b;
      }
      allToTrans.pop_front();

      double rate = 0.0;
      cout << "(((((((((((((((: " << r << endl;
      cout << "(((((((((((((((: " << t << endl;
      if(t > 0.0)
      {
        rate = r/t;
      }
      else
      {
        rate = 0.0;
      }
      dataFile1 << r << endl;
      dataFile2 << rate << endl;
      dataFile1.close();
      dataFile2.close();
    }
    // remainToTrans
    // if(count > 0)
    //     reward2 += sum/count;
    if(tx_total > 0)
    {
      
      reward4 = tx_right/tx_total;
      cout << endl;
      cout << "(" << stepCounter-3 << "时刻)tx_right: " << tx_right << endl;
      cout << "(" << stepCounter-3 << "时刻)tx_total: " << tx_total << endl; 
    }  
    else
    {
      reward4 = 0;
    }
      
    rewardParameters.erase(rewardParameters.begin());
  }
  cout << "(" << stepCounter-3 << "时刻)reward: " << reward << endl;
  cout << "(" << stepCounter-3 << "时刻)reward1: " << reward1 << endl;
  cout << "(" << stepCounter-3 << "时刻)reward2: " << reward2 << endl;
  cout << "(" << stepCounter-3 << "时刻)reward3: " << reward3 << endl;
  cout << "(" << stepCounter-3 << "时刻)reward4: " << reward4 << endl;
  if(dacnum > 0)
    reward = reward2/dacnum;

  // dataFile1 << reward << endl;
  // dataFile1.close();



  if(reward4 == 0)
    reward4 = 0.001;
  return reward4;//加一个baseline 使吞吐率低于0.5为负奖励
  
}

//获取完成调度后的rlcbuffer信息
void rlc()
{
  cout << endl;
    cout << "(" << stepCounter << "时刻)调度后的rlcbuffer: " << endl;
    for (uint32_t i = 0; i < enbDevs.GetN(); i++)
    {
      PointerValue ptr;
      enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetCcMap ().at (0)->GetAttribute (
          "FfMacScheduler", ptr);
     Ptr<LteEnbRrc> enbRrc = enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetRrc ();

      Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler> ();
      Ptr<CqaFfMacScheduler> pff = ff->GetObject<CqaFfMacScheduler> ();

      for (std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
               it = pff->m_rlcBufferReq.begin ();
           it != pff->m_rlcBufferReq.end (); it++)
        {

          cout << i+1 << "-" << it->first.m_rnti  <<  endl;
          cout << "---时延:" << it->second.m_rlcTransmissionQueueHolDelay << " ---tx:" << it->second.m_rlcTransmissionQueueSize  << endl;
          map<uint32_t, ns3::FfMacSchedSapProvider::SchedDlRlcBufferReqParameters> rlcbuffer = rewardParameters.back()[i].rlcbuffer;
          if(rlcbuffer.find(it->first.m_rnti) != rlcbuffer.end() && rlcbuffer.find(it->first.m_rnti)->second.m_rlcTransmissionQueueSize > 0)
          {
            double tx = (double)rlcbuffer.find(it->first.m_rnti)->second.m_rlcTransmissionQueueSize - it->second.m_rlcTransmissionQueueSize;
            // cout << "-----------tx: "<< tx << endl;
            rewardParameters.back()[i].rlcrxstate.insert(pair<uint32_t, double>(it->first.m_rnti, tx));
          }
          
        }
    }

}
//获取sinr
void sinr()
{
  cout << endl;
  cout <<"(" << stepCounter-2 << "时刻的)sinr: " << endl;
  //获取sinr
  for(uint32_t k = 0; k < enbDevs.GetN(); k++)
  {
    map<uint32_t, SpectrumValue> sinr;
    for(uint32_t i = 0; i < ueDevs.GetN(); i++)
    {
      Ptr<LteSpectrumPhy> interf = ueDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetPhy()->GetDownlinkSpectrumPhy();

      uint32_t rnti =  ueDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetRnti();
      uint32_t cellid = ueDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetCellId();
      if(interf-> m_interferenceData->m_receiving && (Simulator::Now () > interf-> m_interferenceData-> m_lastChangeTime) && cellid == k+1)
      {
        
        SpectrumValue allSignals = *(interf-> m_interferenceData->m_allSignals);
        SpectrumValue rxSignal = *(interf-> m_interferenceData->m_rxSignal);
        SpectrumValue noise = *(interf-> m_interferenceData->m_noise);
        SpectrumValue interference = allSignals-rxSignal+noise;
        SpectrumValue sinr_ = rxSignal/interference;
        list<std::vector<rParameters>>::iterator it = ++rewardParameters.begin();
        uint32_t bitmap = (*it)[k].peruerbbitmap.find(rnti)->second;
        uint32_t mask = 1;
        uint32_t count = 0;
        for(uint32_t x = 0; x < bandwidth/rbgSize; x++)
        {
          if(((mask<<x) & bitmap) == 0)
          {
            for(uint32_t l = 0; l < rbgSize; l++)
            {
              sinr_[rbgSize*x+l] = 0;
            }
            count += 1;
          }
        }
        if(count < bandwidth/rbgSize)
        {
          cout << "cellid: " << cellid << endl;
          cout << "rnti: " << rnti << endl;
          cout << "sinr: " << sinr_ << endl;

        }
        sinr.insert(pair<uint32_t, SpectrumValue>(ueDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetRnti(), sinr_));
      }

    }
    rewardParameters.back()[k].sinr = sinr;
  }
}
/*
ns3中的类似gym中的step函数
 */
void
ScheduleNextStateRead (double envStepTime, Ptr<OpenGymInterface> openGym)
{
  Simulator::Schedule (NanoSeconds (1), &rlc);//通过Schedule事件调度机制获取rlcbuffer
  Simulator::Schedule (NanoSeconds (999999), &sinr);//通过Schedule事件调度机制在每个tti结束时获取sinr
  Simulator::Schedule (Seconds (envStepTime), &ScheduleNextStateRead, envStepTime, openGym);
  openGym->NotifyCurrentState ();
}






int
main (int argc, char *argv[])
{
  // Parameters of the scenario
  uint32_t simSeed = 88;
  double simulationTime = 30; //seconds
  double envStepTime = 0.001; //seconds, ns3gym env step time interval
  uint32_t openGymPort = 9999;
  uint32_t testArg = 0;
  CommandLine cmd;
  // required parameters for OpenGym interface
  cmd.AddValue ("openGymPort", "Port number for OpenGym env. Default: 5555", openGymPort);
  cmd.AddValue ("simSeed", "Seed for random generator. Default: 1", simSeed);
  // optional parameters
  cmd.AddValue ("simTime", "Simulation time in seconds. Default: 10s", simulationTime);
  cmd.AddValue ("testArg", "Extra simulation argument. Default: 0", testArg);
  //cmd.Parse (argc, argv);
  

  
  double radius = 5000.0;//基站小区半径
  double high = 1000;//基站高度
  cmd.AddValue ("numberOfRandomUes", "Number of UEs", numberOfRandomUes);

  //Input the parameters from the config txt file
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line
  cmd.Parse (argc, argv);

 
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (bandwidth));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (bandwidth));
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));

  // lteHelper->SetFfrAlgorithmType ("ns3::LteFrSoftAlgorithm");//软频率服用默认为三色复用
  lteHelper->SetSchedulerType("ns3::CqaFfMacScheduler");
  
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.0)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
      ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());

  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer enbNodes;
  NodeContainer ueNodes;

  enbNodes.Create (numberOfenb);
  ueNodes.Create (numberOfRandomUes);

  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  
 //基站的位置

  enbPositionAlloc->Add (Vector (sqrt(3) * radius/2, 0.0, high)); //eNB1

  enbPositionAlloc->Add (Vector (-sqrt(3) * radius/2, 0.0, high)); //eNB2

  enbPositionAlloc->Add (Vector (0.0, 3 * radius/2, high)); //eNB3




  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (enbPositionAlloc);
  mobility.Install (enbNodes);



  Ptr<RandomBoxPositionAllocator> randomUePositionAlloc =
      CreateObject<RandomBoxPositionAllocator> ();
  Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable> ();
  xVal->SetAttribute ("Min", DoubleValue (-6000.0));
  xVal->SetAttribute ("Max", DoubleValue (6000.0));
  randomUePositionAlloc->SetAttribute ("X", PointerValue (xVal));
  Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable> ();
  yVal->SetAttribute ("Min", DoubleValue (-1000.0));
  yVal->SetAttribute ("Max", DoubleValue (6000.0));
  randomUePositionAlloc->SetAttribute ("Y", PointerValue (yVal));
  Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable> ();
  zVal->SetAttribute ("Min", DoubleValue (0));
  zVal->SetAttribute ("Max", DoubleValue (1));
  randomUePositionAlloc->SetAttribute ("Z", PointerValue (zVal));
  mobility.SetPositionAllocator (randomUePositionAlloc);
  mobility.Install (ueNodes);

  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueDevs = lteHelper->InstallUeDevice (ueNodes);

  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

  std::cout << " simTime: " << simulationTime << " | all ues is: " << ueNodes.GetN () << std::endl;

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      Ptr<Ipv4StaticRouting> ueStaticRouting =
          ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  lteHelper->AttachToClosestEnb (ueDevs, enbDevs);
  // lteHelper->Attach(ueDevs,enbDevs.Get(0));

  uint16_t dlPort = 10000;
  uint16_t ulPort = 20000;
  double lambda = 70;
  Ptr<ExponentialRandomVariable> startTimeSeconds = CreateObject<ExponentialRandomVariable> ();//产生服从负指数分布的随机数
  startTimeSeconds->SetAttribute ("Mean", DoubleValue (1/lambda));

  Ptr<UniformRandomVariable> stopTimeSeconds = CreateObject<UniformRandomVariable> ();
  stopTimeSeconds->SetAttribute ("Min", DoubleValue (2));
  stopTimeSeconds->SetAttribute ("Max", DoubleValue (4));
  vector<uint32_t> interval(numberOfRandomUes,0);
  
  for(uint32_t p = 0; p < numberOfRandomUes; p++)
  {
    cout << startTimeSeconds->GetValue () << endl;
    interval[p] = (uint32_t)(1000*startTimeSeconds->GetValue ());//把随机数的单位从s转为ms
    
  }
  
  vector<uint32_t> arriveTime(numberOfRandomUes+1,0);

  for(uint32_t p = 1; p < numberOfRandomUes; p++)
  {
    arriveTime[p] = arriveTime[p-1] + interval[p];//依次累加可得业务到达的时间序列
  }
  
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;

  // vector<uint32_t> interval = {443,158,677,397,243,913,221,482,1877,1445,376,1704,570,627,259,324,222,1469,83	,888	,3441	,40	,534,	1185	,272,	370	,314	,18	,1127	,557	,1108	,1310	,54	,78	,313,	1559	,70	,1133	,366	,1343	,1470,	1179,	2261	,737,	1561	,308,	315	,1787	,1324	,1067};
  // vector<uint32_t> arriveTime = {0	,443,	600	,1277,	1674,	1917	,2830,	3051,	3534,	5411	,6856,	7232,	8936,	9505,	10133,	10392,	10716,	10938	,12407,	12490,	13378	,16819,	16859	,17393,	18577	,18850,	19220,	19534	,19551,	20678	,21235,	22343	,23653,	23708,	23785,	24098,	25657,	25728	,26861	,27226,	28569	,30039	,31218,	33479,	34216,	35777	,36085,	36400,	38187};
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      Ptr<Ipv4StaticRouting> ueStaticRouting =
      ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      for (uint32_t b = 0; b < 1; ++b)
        {
          ++dlPort;
          ++ulPort;


          // UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
          // dlClientHelper.SetAttribute ("MaxPackets", UintegerValue (1000000));
          // dlClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (1)));



          OnOffHelper dlClientHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (ueIpIfaces.GetAddress (u), dlPort));
          cout << "ue-" << u <<  ": " << ueIpIfaces.GetAddress (u) << endl;
          dlClientHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.09]"));   
          dlClientHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          dlClientHelper.SetAttribute("PacketSize", UintegerValue (256));
          dlClientHelper.SetAttribute("DataRate", DataRateValue (DataRate ("100kb/s")));
          dlClientHelper.SetAttribute("MaxBytes", UintegerValue (1024000));
          // dlClientHelper.SetAttribute ("MaxPackets", UintegerValue (100));
          // dlClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (20.0)));
          clientApps.Add (dlClientHelper.Install (remoteHost));
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), dlPort));
                               
          serverApps.Add (dlPacketSinkHelper.Install (ue));


          

          OnOffHelper ulClientHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (remoteHostAddr, ulPort));
          ulClientHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.09]"));   
          ulClientHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          


          // UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
          // ulClientHelper.SetAttribute ("MaxPackets", UintegerValue (1000000));
          // ulClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (1)));
          // ulClientHelper.SetAttribute ("MaxPackets", UintegerValue (100));
          // ulClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (5.0)));
          clientApps.Add (ulClientHelper.Install (ue));
          PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), ulPort));
          serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

          Ptr<EpcTft> tft = Create<EpcTft> ();
          EpcTft::PacketFilter dlpf;
          dlpf.localPortStart = dlPort;
          dlpf.localPortEnd = dlPort;
          tft->Add (dlpf);

          EpcTft::PacketFilter ulpf;
          ulpf.remotePortStart = ulPort;
          ulpf.remotePortEnd = ulPort;
          tft->Add (ulpf);
          
          // if (u % 2 == 0)
          //   {
              enum EpsBearer::Qci q = EpsBearer::GBR_GAMING;
              EpsBearer bearer (q);
             
              lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), EpsBearer(EpsBearer:: GBR_CONV_VOICE ), tft);
          //   }
          // else
          //   {
          //     lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), EpsBearer(EpsBearer::GBR_GAMING), tft);
          //   }

          
        }
         Time startTime; 
          Time stopTime = Seconds (stopTimeSeconds->GetValue ());
          cout << MilliSeconds(arriveTime[u]) << endl;
          startTime = MilliSeconds(arriveTime[u]);
          serverApps.Get(2*u)->SetStartTime(startTime);
          serverApps.Get(2*u+1)->SetStartTime(startTime);
          clientApps.Get(2*u)->SetStartTime (startTime);
          clientApps.Get(2*u+1)->SetStartTime (startTime);

          serverApps.Get(2*u)->SetStopTime (startTime+stopTime );
          serverApps.Get(2*u+1)->SetStopTime (startTime+stopTime );
          clientApps.Get(2*u)->SetStopTime (startTime+stopTime );
          clientApps.Get(2*u+1)->SetStopTime (startTime+stopTime );
          
          

    }
    monitor = flowmon.Install(ueNodes);
  
  flowmon.InstallAll ();
  lteHelper->EnableTraces ();

  // OpenGym Env
  Ptr<OpenGymInterface> openGym = CreateObject<OpenGymInterface> (openGymPort);
  openGym->SetGetActionSpaceCb (MakeCallback (&MyGetActionSpace));
  openGym->SetGetObservationSpaceCb (MakeCallback (&MyGetObservationSpace));
  openGym->SetGetGameOverCb (MakeCallback (&MyGetGameOver));
  openGym->SetGetObservationCb (MakeCallback (&MyGetObservation));
  openGym->SetGetRewardCb (MakeCallback (&MyGetReward));
  openGym->SetGetExtraInfoCb (MakeCallback (&MyGetExtraInfo));
  openGym->SetExecuteActionsCb (MakeCallback (&MyExecuteActions));
  Simulator::Schedule (Seconds (0.0), &ScheduleNextStateRead, envStepTime, openGym);
  NS_LOG_UNCOND ("Simulation start");
  Simulator::Stop (Seconds (simulationTime));
  AnimationInterface anim ("sim.xml");
  Simulator::Run ();

  NS_LOG_UNCOND ("Simulation stop");

  openGym->NotifySimulationEnd ();
  Simulator::Destroy ();
  return 0;
}
