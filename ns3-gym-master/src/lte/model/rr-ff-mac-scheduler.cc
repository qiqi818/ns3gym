/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 */
#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/math.h>

#include <ns3/simulator.h>
#include <ns3/lte-amc.h>
#include <ns3/pss-ff-mac-scheduler.h>
#include <ns3/lte-vendor-specific-parameters.h>
#include <ns3/boolean.h>
#include <cfloat>
#include <set>
#include <ns3/string.h>
#include <algorithm>
#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/math.h>

#include <ns3/simulator.h>
#include <ns3/lte-amc.h>

#include <ns3/ff-mac-common.h>
#include <ns3/lte-vendor-specific-parameters.h>
#include <ns3/boolean.h>
#include <cfloat>
#include <set>
#include <stdexcept>
#include <ns3/integer.h>
#include <ns3/string.h>


#include <cfloat>
#include <set>
#include <climits>


#include <ns3/rr-ff-mac-scheduler.h>
// #include <ns3/cqa-ff-mac-scheduler.h>
#include <ns3/simulator.h>
#include <ns3/lte-common.h>
#include <ns3/lte-vendor-specific-parameters.h>
#include <ns3/boolean.h>


#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/math.h>

#include <ns3/simulator.h>
#include <ns3/lte-amc.h>
// #include <ns3/fdbet-ff-mac-scheduler.h>
#include <ns3/lte-vendor-specific-parameters.h>
#include <ns3/boolean.h>
#include <set>
#include <cfloat>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RrFfMacScheduler");

/// Type 0 allocation RBG
static const int Type0AllocationRbg[4] = {
  10,       // RGB size 1
  26,       // RGB size 2
  63,       // RGB size 3
  110       // RGB size 4
};  // see table 7.1.6.1-1 of 36.213




NS_OBJECT_ENSURE_REGISTERED (RrFfMacScheduler);

/// qos_rb_and_CQI_assigned_to_lc
struct qos_rb_and_CQI_assigned_to_lc
{
  uint16_t resource_block_index;   ///< Resource block indexHOL_GROUP_index
  uint8_t  cqi_value_for_lc;       ///< CQI indicator value
};


/**
 * CQI value comparator function
 * \param key1 the first item
 * \param key2 the second item
 * \returns true if the first item is > the second item
 */
bool CQIValueDescComparator_ (uint8_t key1, uint8_t key2)
{
  return key1>key2;
}

/**
 * CGA group comparator function
 * \param key1 the first item
 * \param key2 the second item
 * \returns true if the first item is > the second item
 */
bool CqaGroupDescComparator_ (int key1, int key2)
{
  return key1>key2;
}






/// CQI value typedef
typedef uint8_t CQI_value;
/// RBG index typedef
typedef int RBG_index;
/// HOL group typedef
typedef int HOL_group;

/// CQI value map typedef
typedef std::map<CQI_value,LteFlowId_t,bool(*)(uint8_t,uint8_t)> t_map_CQIToUE; //sorted
/// RBG index map typedef
typedef std::map<RBG_index,t_map_CQIToUE> t_map_RBGToCQIsSorted;
/// HOL group map typedef
typedef std::map<HOL_group,t_map_RBGToCQIsSorted> t_map_HOLGroupToRBGs;

/// CQI value map iterator typedef
typedef std::map<CQI_value,LteFlowId_t,bool(*)(uint8_t,uint8_t)>::iterator t_it_CQIToUE; //sorted
/// RBG index map iterator typedef
typedef std::map<RBG_index,t_map_CQIToUE>::iterator t_it_RBGToCQIsSorted;
/// HOL group map iterator typedef
typedef std::map<HOL_group,t_map_RBGToCQIsSorted>::iterator t_it_HOLGroupToRBGs;

/// HOL group map typedef
typedef std::multimap<HOL_group,std::set<LteFlowId_t>,bool(*)(int,int)> t_map_HOLgroupToUEs;
/// HOL group multi map iterator typedef
typedef std::map<HOL_group,std::set<LteFlowId_t> >::iterator t_it_HOLgroupToUEs;

//typedef std::map<RBG_index,CQI_value>  map_RBG_to_CQI;
//typedef std::map<LteFlowId_t,map_RBG_to_CQI> map_flowId_to_CQI_map;
/**
 * CQA key comparator
 * \param key1 the first item
 * \param key2 the second item
 * \returns true if the first item > the second item
 */  
bool CqaKeyDescComparator_ (uint16_t key1, uint16_t key2)
{
  return key1>key2;
}



RrFfMacScheduler::RrFfMacScheduler ()
{
  if(m_flag == 1)
  {
    // std::cout << "执行1--------------------" << std::endl;
    m_cschedSapUser = 0;
    m_schedSapUser = 0;
    m_timeWindow = 99.0;
    m_nextRntiUl = 0;
    m_nextRntiDl = 0;

    m_amc = CreateObject <LteAmc> ();
    m_cschedSapProvider = new MemberCschedSapProvider<RrFfMacScheduler> (this);
    m_schedSapProvider = new MemberSchedSapProvider<RrFfMacScheduler> (this);
    m_ffrSapProvider = 0;
    m_ffrSapUser = new MemberLteFfrSapUser<RrFfMacScheduler> (this);
  }
  else if(m_flag == 2)
  {
    m_cschedSapUser = 0;
    m_schedSapUser = 0;
    m_timeWindow = 99.0;
    m_nextRntiUl = 0;
    m_nextRntiDl = 0;

    m_amc = CreateObject <LteAmc> ();
    m_cschedSapProvider = new MemberCschedSapProvider<RrFfMacScheduler> (this);
    m_schedSapProvider = new MemberSchedSapProvider<RrFfMacScheduler> (this);
    m_ffrSapProvider = 0;
    m_ffrSapUser = new MemberLteFfrSapUser<RrFfMacScheduler> (this);
  }
  else if(m_flag == 3)
  {
    m_cschedSapUser = 0;
    m_schedSapUser = 0;
    m_timeWindow = 99.0;
    m_nextRntiUl = 0;
    m_nextRntiDl = 0;

    m_amc = CreateObject <LteAmc> ();
    m_cschedSapProvider = new MemberCschedSapProvider<RrFfMacScheduler> (this);
    m_schedSapProvider = new MemberSchedSapProvider<RrFfMacScheduler> (this);
    m_ffrSapProvider = 0;
    m_ffrSapUser = new MemberLteFfrSapUser<RrFfMacScheduler> (this);
  }
  else
  {
    // std::cout << "执行0----------------------------" << std::endl;
    m_cschedSapUser = 0;
    m_schedSapUser = 0;
    m_timeWindow = 99.0;
    m_nextRntiDl = 0;
    m_nextRntiUl = 0;

    m_amc = CreateObject <LteAmc> ();
    m_cschedSapProvider = new MemberCschedSapProvider<RrFfMacScheduler> (this);
    m_schedSapProvider = new MemberSchedSapProvider<RrFfMacScheduler> (this);
    m_ffrSapProvider = 0;
    m_ffrSapUser = new MemberLteFfrSapUser<RrFfMacScheduler> (this);
  }
  
  
}

RrFfMacScheduler::~RrFfMacScheduler ()
{
  NS_LOG_FUNCTION (this);
}

void
RrFfMacScheduler::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_dlHarqProcessesDciBuffer.clear ();
  m_dlHarqProcessesTimer.clear ();
  m_dlHarqProcessesRlcPduListBuffer.clear ();
  m_dlInfoListBuffered.clear ();
  m_ulHarqCurrentProcessId.clear ();
  m_ulHarqProcessesStatus.clear ();
  m_ulHarqProcessesDciBuffer.clear ();
  delete m_cschedSapProvider;
  delete m_schedSapProvider;
  delete m_ffrSapUser;
  
}

TypeId
RrFfMacScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RrFfMacScheduler")
    .SetParent<FfMacScheduler> ()
    .SetGroupName("Lte")
    .AddConstructor<RrFfMacScheduler> ()
    .AddAttribute ("CqiTimerThreshold",
                   "The number of TTIs a CQI is valid (default 1000 - 1 sec.)",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&RrFfMacScheduler::m_cqiTimersThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("HarqEnabled",
                   "Activate/Deactivate the HARQ [by default is active].",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RrFfMacScheduler::m_harqOn),
                   MakeBooleanChecker ())
    .AddAttribute ("UlGrantMcs",
                   "The MCS of the UL grant, must be [0..15] (default 0)",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RrFfMacScheduler::m_ulGrantMcs),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("CqaMetric",
                   "CqaFfMacScheduler metric type that can be: CqaFf, CqaPf",
                   StringValue ("CqaFf"),
                   MakeStringAccessor (&RrFfMacScheduler::m_CqaMetric),
                   MakeStringChecker ())
    .AddAttribute ("m_flag",
                   "The type of scheduler",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RrFfMacScheduler::m_flag),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("PssFdSchedulerType",
                   "FD scheduler in PSS (default value is PFsch)",
                   StringValue ("PFsch"),
                   MakeStringAccessor (&RrFfMacScheduler::m_fdSchedulerType),
                   MakeStringChecker ())
    .AddAttribute ("nMux",
                   "The number of UE selected by TD scheduler (default value is 0)",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RrFfMacScheduler::m_nMux),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}



void
RrFfMacScheduler::SetFfMacCschedSapUser (FfMacCschedSapUser* s)
{
  m_cschedSapUser = s;
}

void
RrFfMacScheduler::SetFfMacSchedSapUser (FfMacSchedSapUser* s)
{
  m_schedSapUser = s;
}

FfMacCschedSapProvider*
RrFfMacScheduler::GetFfMacCschedSapProvider ()
{
  return m_cschedSapProvider;
}

FfMacSchedSapProvider*
RrFfMacScheduler::GetFfMacSchedSapProvider ()
{
  return m_schedSapProvider;
}

void
RrFfMacScheduler::SetLteFfrSapProvider (LteFfrSapProvider* s)
{
  m_ffrSapProvider = s;
}

LteFfrSapUser*
RrFfMacScheduler::GetLteFfrSapUser ()
{
  return m_ffrSapUser;
}

void
RrFfMacScheduler::DoCschedCellConfigReq (const struct FfMacCschedSapProvider::CschedCellConfigReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  // Read the subset of parameters used
  m_cschedCellConfig = params;
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  FfMacCschedSapUser::CschedUeConfigCnfParameters cnf;
  cnf.m_result = SUCCESS;
  m_cschedSapUser->CschedUeConfigCnf (cnf);
  return;
}

void
RrFfMacScheduler::DoCschedUeConfigReq (const struct FfMacCschedSapProvider::CschedUeConfigReqParameters& params)
{
  // std::cout << "执行了DoCschedUeConfigReq" << std::endl;
  if(m_flag == 0)
  {
    // std::cout << "执行0-----" << std::endl;
  NS_LOG_FUNCTION (this << " RNTI " << params.m_rnti << " txMode " << (uint16_t)params.m_transmissionMode);
  std::map <uint16_t,uint8_t>::iterator it = m_uesTxMode.find (params.m_rnti);
  if (it == m_uesTxMode.end ())
    {
      m_uesTxMode.insert (std::pair <uint16_t, double> (params.m_rnti, params.m_transmissionMode));
      // generate HARQ buffers
      m_dlHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      DlHarqProcessesStatus_t dlHarqPrcStatus;
      dlHarqPrcStatus.resize (8,0);
      m_dlHarqProcessesStatus.insert (std::pair <uint16_t, DlHarqProcessesStatus_t> (params.m_rnti, dlHarqPrcStatus));
      DlHarqProcessesTimer_t dlHarqProcessesTimer;
      dlHarqProcessesTimer.resize (8,0);
      m_dlHarqProcessesTimer.insert (std::pair <uint16_t, DlHarqProcessesTimer_t> (params.m_rnti, dlHarqProcessesTimer));
      DlHarqProcessesDciBuffer_t dlHarqdci;
      dlHarqdci.resize (8);
      m_dlHarqProcessesDciBuffer.insert (std::pair <uint16_t, DlHarqProcessesDciBuffer_t> (params.m_rnti, dlHarqdci));
      DlHarqRlcPduListBuffer_t dlHarqRlcPdu;
      dlHarqRlcPdu.resize (2);
      dlHarqRlcPdu.at (0).resize (8);
      dlHarqRlcPdu.at (1).resize (8);
      m_dlHarqProcessesRlcPduListBuffer.insert (std::pair <uint16_t, DlHarqRlcPduListBuffer_t> (params.m_rnti, dlHarqRlcPdu));
      m_ulHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      UlHarqProcessesStatus_t ulHarqPrcStatus;
      ulHarqPrcStatus.resize (8,0);
      m_ulHarqProcessesStatus.insert (std::pair <uint16_t, UlHarqProcessesStatus_t> (params.m_rnti, ulHarqPrcStatus));
      UlHarqProcessesDciBuffer_t ulHarqdci;
      ulHarqdci.resize (8);
      m_ulHarqProcessesDciBuffer.insert (std::pair <uint16_t, UlHarqProcessesDciBuffer_t> (params.m_rnti, ulHarqdci));
    }
  else
    {
      (*it).second = params.m_transmissionMode;
    }
  return;
  }
  else if(m_flag == 2)
  {
    // std::cout << "运行了xx" << std::endl;
    NS_LOG_FUNCTION (this << " RNTI " << params.m_rnti << " txMode " << (uint16_t)params.m_transmissionMode);
    std::map <uint16_t,uint8_t>::iterator it = m_uesTxMode.find (params.m_rnti);
    if (it == m_uesTxMode.end ())
    {
      m_uesTxMode.insert (std::pair <uint16_t, double> (params.m_rnti, params.m_transmissionMode));
      // generate HARQ buffers
      m_dlHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      DlHarqProcessesStatus_t dlHarqPrcStatus;
      dlHarqPrcStatus.resize (8,0);
      m_dlHarqProcessesStatus.insert (std::pair <uint16_t, DlHarqProcessesStatus_t> (params.m_rnti, dlHarqPrcStatus));
      DlHarqProcessesTimer_t dlHarqProcessesTimer;
      dlHarqProcessesTimer.resize (8,0);
      m_dlHarqProcessesTimer.insert (std::pair <uint16_t, DlHarqProcessesTimer_t> (params.m_rnti, dlHarqProcessesTimer));
      DlHarqProcessesDciBuffer_t dlHarqdci;
      dlHarqdci.resize (8);
      m_dlHarqProcessesDciBuffer.insert (std::pair <uint16_t, DlHarqProcessesDciBuffer_t> (params.m_rnti, dlHarqdci));
      DlHarqRlcPduListBuffer_t dlHarqRlcPdu;
      dlHarqRlcPdu.resize (2);
      dlHarqRlcPdu.at (0).resize (8);
      dlHarqRlcPdu.at (1).resize (8);
      m_dlHarqProcessesRlcPduListBuffer.insert (std::pair <uint16_t, DlHarqRlcPduListBuffer_t> (params.m_rnti, dlHarqRlcPdu));
      m_ulHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      UlHarqProcessesStatus_t ulHarqPrcStatus;
      ulHarqPrcStatus.resize (8,0);
      m_ulHarqProcessesStatus.insert (std::pair <uint16_t, UlHarqProcessesStatus_t> (params.m_rnti, ulHarqPrcStatus));
      UlHarqProcessesDciBuffer_t ulHarqdci;
      ulHarqdci.resize (8);
      m_ulHarqProcessesDciBuffer.insert (std::pair <uint16_t, UlHarqProcessesDciBuffer_t> (params.m_rnti, ulHarqdci));
    }
  else
    {
      (*it).second = params.m_transmissionMode;
    }
  return;
  }
  else if(m_flag == 3)
  {
    NS_LOG_FUNCTION (this << " RNTI " << params.m_rnti << " txMode " << (uint16_t)params.m_transmissionMode);
  std::map <uint16_t,uint8_t>::iterator it = m_uesTxMode.find (params.m_rnti);
  if (it == m_uesTxMode.end ())
    {
      m_uesTxMode.insert (std::pair <uint16_t, double> (params.m_rnti, params.m_transmissionMode));
      // generate HARQ buffers
      m_dlHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      DlHarqProcessesStatus_t dlHarqPrcStatus;
      dlHarqPrcStatus.resize (8,0);
      m_dlHarqProcessesStatus.insert (std::pair <uint16_t, DlHarqProcessesStatus_t> (params.m_rnti, dlHarqPrcStatus));
      DlHarqProcessesTimer_t dlHarqProcessesTimer;
      dlHarqProcessesTimer.resize (8,0);
      m_dlHarqProcessesTimer.insert (std::pair <uint16_t, DlHarqProcessesTimer_t> (params.m_rnti, dlHarqProcessesTimer));
      DlHarqProcessesDciBuffer_t dlHarqdci;
      dlHarqdci.resize (8);
      m_dlHarqProcessesDciBuffer.insert (std::pair <uint16_t, DlHarqProcessesDciBuffer_t> (params.m_rnti, dlHarqdci));
      DlHarqRlcPduListBuffer_t dlHarqRlcPdu;
      dlHarqRlcPdu.resize (2);
      dlHarqRlcPdu.at (0).resize (8);
      dlHarqRlcPdu.at (1).resize (8);
      m_dlHarqProcessesRlcPduListBuffer.insert (std::pair <uint16_t, DlHarqRlcPduListBuffer_t> (params.m_rnti, dlHarqRlcPdu));
      m_ulHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      UlHarqProcessesStatus_t ulHarqPrcStatus;
      ulHarqPrcStatus.resize (8,0);
      m_ulHarqProcessesStatus.insert (std::pair <uint16_t, UlHarqProcessesStatus_t> (params.m_rnti, ulHarqPrcStatus));
      UlHarqProcessesDciBuffer_t ulHarqdci;
      ulHarqdci.resize (8);
      m_ulHarqProcessesDciBuffer.insert (std::pair <uint16_t, UlHarqProcessesDciBuffer_t> (params.m_rnti, ulHarqdci));
    }
  else
    {
      (*it).second = params.m_transmissionMode;
    }
  return;
  }
  else
  {
    // std::cout << "执行1" << std::endl;
    NS_LOG_FUNCTION (this << " RNTI " << params.m_rnti << " txMode " << (uint16_t)params.m_transmissionMode);
  std::map <uint16_t,uint8_t>::iterator it = m_uesTxMode.find (params.m_rnti);
  if (it == m_uesTxMode.end ())
    {
      m_uesTxMode.insert (std::pair <uint16_t, uint8_t> (params.m_rnti, params.m_transmissionMode));
      // generate HARQ buffers
      m_dlHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      DlHarqProcessesStatus_t dlHarqPrcStatus;
      dlHarqPrcStatus.resize (8,0);
      m_dlHarqProcessesStatus.insert (std::pair <uint16_t, DlHarqProcessesStatus_t> (params.m_rnti, dlHarqPrcStatus));
      DlHarqProcessesTimer_t dlHarqProcessesTimer;
      dlHarqProcessesTimer.resize (8,0);
      m_dlHarqProcessesTimer.insert (std::pair <uint16_t, DlHarqProcessesTimer_t> (params.m_rnti, dlHarqProcessesTimer));
      DlHarqProcessesDciBuffer_t dlHarqdci;
      dlHarqdci.resize (8);
      m_dlHarqProcessesDciBuffer.insert (std::pair <uint16_t, DlHarqProcessesDciBuffer_t> (params.m_rnti, dlHarqdci));
      DlHarqRlcPduListBuffer_t dlHarqRlcPdu;
      dlHarqRlcPdu.resize (2);
      dlHarqRlcPdu.at (0).resize (8);
      dlHarqRlcPdu.at (1).resize (8);
      m_dlHarqProcessesRlcPduListBuffer.insert (std::pair <uint16_t, DlHarqRlcPduListBuffer_t> (params.m_rnti, dlHarqRlcPdu));
      m_ulHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      UlHarqProcessesStatus_t ulHarqPrcStatus;
      ulHarqPrcStatus.resize (8,0);
      m_ulHarqProcessesStatus.insert (std::pair <uint16_t, UlHarqProcessesStatus_t> (params.m_rnti, ulHarqPrcStatus));
      UlHarqProcessesDciBuffer_t ulHarqdci;
      ulHarqdci.resize (8);
      m_ulHarqProcessesDciBuffer.insert (std::pair <uint16_t, UlHarqProcessesDciBuffer_t> (params.m_rnti, ulHarqdci));
    }
  else
    {
      (*it).second = params.m_transmissionMode;
    }
  return;
  }
}

void
RrFfMacScheduler::DoCschedLcConfigReq (const struct FfMacCschedSapProvider::CschedLcConfigReqParameters& params)
{
  // if(m_flag == 0)
  // {
  //   std::cout << "执行0" << std::endl;
  // NS_LOG_FUNCTION (this);
  // // Not used at this stage (LCs updated by DoSchedDlRlcBufferReq)
  // return;
  // }
  // if(m_flag == 2)
  // {
  //   NS_LOG_FUNCTION (this << " New LC, rnti: "  << params.m_rnti);

  //   std::map <uint16_t, RrsFlowPerf_t>::iterator it;
  //   for (uint16_t i = 0; i < params.m_logicalChannelConfigList.size (); i++)
  //     {
  //       it = m_flowStatsDl.find (params.m_rnti);

  //       if (it == m_flowStatsDl.end ())
  //         {
  //           RrsFlowPerf_t flowStatsDl;
  //           flowStatsDl.flowStart = Simulator::Now ();
  //           flowStatsDl.totalBytesTransmitted = 0;
  //           flowStatsDl.lastTtiBytesTransmitted = 0;
  //           flowStatsDl.lastAveragedThroughput = 1;
  //           m_flowStatsDl.insert (std::pair<uint16_t, RrsFlowPerf_t> (params.m_rnti, flowStatsDl));
  //           RrsFlowPerf_t flowStatsUl;
  //           flowStatsUl.flowStart = Simulator::Now ();
  //           flowStatsUl.totalBytesTransmitted = 0;
  //           flowStatsUl.lastTtiBytesTransmitted = 0;
  //           flowStatsUl.lastAveragedThroughput = 1;
  //           m_flowStatsUl.insert (std::pair<uint16_t, RrsFlowPerf_t> (params.m_rnti, flowStatsUl));
  //         }
  //     }

  //   return;
  // }
  // else if(m_flag == 3)
  // {
  //   NS_LOG_FUNCTION (this << " New LC, rnti: "  << params.m_rnti);

  // std::map <uint16_t, RrsFlowPerf_t>::iterator it;
  // for (uint16_t i = 0; i < params.m_logicalChannelConfigList.size (); i++)
  //   {
  //     it = m_flowStatsDl.find (params.m_rnti);

  //     if (it == m_flowStatsDl.end ())
  //       {
  //         double tbrDlInBytes = params.m_logicalChannelConfigList.at (i).m_eRabGuaranteedBitrateDl / 8;   // byte/s
  //         double tbrUlInBytes = params.m_logicalChannelConfigList.at (i).m_eRabGuaranteedBitrateUl / 8;   // byte/s

  //         RrsFlowPerf_t flowStatsDl;
  //         flowStatsDl.flowStart = Simulator::Now ();
  //         flowStatsDl.totalBytesTransmitted = 0;
  //         flowStatsDl.lastTtiBytesTransmitted = 0;
  //         flowStatsDl.lastAveragedThroughput = 1;
  //         flowStatsDl.secondLastAveragedThroughput = 1;
  //         flowStatsDl.targetThroughput = tbrDlInBytes;
  //         m_flowStatsDl.insert (std::pair<uint16_t, RrsFlowPerf_t> (params.m_rnti, flowStatsDl));
  //         RrsFlowPerf_t flowStatsUl;
  //         flowStatsUl.flowStart = Simulator::Now ();
  //         flowStatsUl.totalBytesTransmitted = 0;
  //         flowStatsUl.lastTtiBytesTransmitted = 0;
  //         flowStatsUl.lastAveragedThroughput = 1;
  //         flowStatsUl.secondLastAveragedThroughput = 1;
  //         flowStatsUl.targetThroughput = tbrUlInBytes;
  //         m_flowStatsUl.insert (std::pair<uint16_t, RrsFlowPerf_t> (params.m_rnti, flowStatsUl));
  //       }
  //     else
  //       {
  //         // update GBR from UeManager::SetupDataRadioBearer ()
  //         double tbrDlInBytes = params.m_logicalChannelConfigList.at (i).m_eRabGuaranteedBitrateDl / 8;   // byte/s
  //         double tbrUlInBytes = params.m_logicalChannelConfigList.at (i).m_eRabGuaranteedBitrateUl / 8;   // byte/s
  //         m_flowStatsDl[(*it).first].targetThroughput = tbrDlInBytes;
  //         m_flowStatsUl[(*it).first].targetThroughput = tbrUlInBytes;
  //       }
  //   }

  // return;
  // }
  // else
  // {
    // std::cout << "执行1" << std::endl;
    NS_LOG_FUNCTION (this << " New LC, rnti: "  << params.m_rnti);

    NS_LOG_FUNCTION ("LC configuration. Number of LCs:"<<params.m_logicalChannelConfigList.size ());

    // m_reconfigureFlat indicates if this is a reconfiguration or new UE is added, table  4.1.5 in LTE MAC scheduler specification
    if (params.m_reconfigureFlag)
      {
        std::vector <struct LogicalChannelConfigListElement_s>::const_iterator lcit;

        for(lcit = params.m_logicalChannelConfigList.begin (); lcit!= params.m_logicalChannelConfigList.end (); lcit++)
          {
            LteFlowId_t flowid = LteFlowId_t (params.m_rnti,lcit->m_logicalChannelIdentity);

            if (m_ueLogicalChannelsConfigList.find (flowid) == m_ueLogicalChannelsConfigList.end ())
              {
                NS_LOG_ERROR ("UE logical channels can not be reconfigured because it was not configured before.");
              }
            else
              {
                m_ueLogicalChannelsConfigList.find (flowid)->second = *lcit;
              }
          }

    }    // else new UE is added
  else
    {
      std::vector <struct LogicalChannelConfigListElement_s>::const_iterator lcit;

      for (lcit = params.m_logicalChannelConfigList.begin (); lcit != params.m_logicalChannelConfigList.end (); lcit++)
        {
          LteFlowId_t flowId = LteFlowId_t (params.m_rnti,lcit->m_logicalChannelIdentity);
          m_ueLogicalChannelsConfigList.insert (std::pair<LteFlowId_t, LogicalChannelConfigListElement_s>(flowId,*lcit));
        }
    }


  std::map <uint16_t, RrsFlowPerf_t>::iterator it;

  for (uint16_t i = 0; i < params.m_logicalChannelConfigList.size (); i++)
    {
      it = m_flowStatsDl.find (params.m_rnti);

      if (it == m_flowStatsDl.end ())
        {
          double tbrDlInBytes = params.m_logicalChannelConfigList.at (i).m_eRabGuaranteedBitrateDl / 8;   // byte/s
          double tbrUlInBytes = params.m_logicalChannelConfigList.at (i).m_eRabGuaranteedBitrateUl / 8;   // byte/s

          RrsFlowPerf_t flowStatsDl;
          flowStatsDl.flowStart = Simulator::Now ();
          flowStatsDl.totalBytesTransmitted = 0;
          flowStatsDl.lastTtiBytesTransmitted = 0;
          flowStatsDl.lastAveragedThroughput = 1;
          flowStatsDl.secondLastAveragedThroughput = 1;
          flowStatsDl.targetThroughput = tbrDlInBytes;
          m_flowStatsDl.insert (std::pair<uint16_t, RrsFlowPerf_t> (params.m_rnti, flowStatsDl));
          RrsFlowPerf_t flowStatsUl;
          flowStatsUl.flowStart = Simulator::Now ();
          flowStatsUl.totalBytesTransmitted = 0;
          flowStatsUl.lastTtiBytesTransmitted = 0;
          flowStatsUl.lastAveragedThroughput = 1;
          flowStatsUl.secondLastAveragedThroughput = 1;
          flowStatsUl.targetThroughput = tbrUlInBytes;
          m_flowStatsUl.insert (std::pair<uint16_t, RrsFlowPerf_t> (params.m_rnti, flowStatsUl));
        }
      else
        {
          // update GBR from UeManager::SetupDataRadioBearer ()
          double tbrDlInBytes = params.m_logicalChannelConfigList.at (i).m_eRabGuaranteedBitrateDl / 8;   // byte/s
          double tbrUlInBytes = params.m_logicalChannelConfigList.at (i).m_eRabGuaranteedBitrateUl / 8;   // byte/s
          m_flowStatsDl[(*it).first].targetThroughput = tbrDlInBytes;
          m_flowStatsUl[(*it).first].targetThroughput = tbrUlInBytes;

        }
    }

  return;
  // }
  
}

void
RrFfMacScheduler::DoCschedLcReleaseReq (const struct FfMacCschedSapProvider::CschedLcReleaseReqParameters& params)
{
  // if(m_flag == 0)
  // {
  //   std::cout << "执行0" << std::endl;
  // NS_LOG_FUNCTION (this);
  //   for (uint16_t i = 0; i < params.m_logicalChannelIdentity.size (); i++)
  //   {
  //    std::list<FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it = m_rlcBufferReq.begin ();
  //     while (it!=m_rlcBufferReq.end ())
  //       {
  //         if (((*it).m_rnti == params.m_rnti)&&((*it).m_logicalChannelIdentity == params.m_logicalChannelIdentity.at (i)))
  //           {
  //             it = m_rlcBufferReq.erase (it);
  //           }
  //         else
  //           {
  //             it++;
  //           }
  //       }
  //   }
  // return;
  // }
  // else if(m_flag == 2)
  // {
    
  //   NS_LOG_FUNCTION (this);
  //   for (uint16_t i = 0; i < params.m_logicalChannelIdentity.size (); i++)
  //     {
  //       std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it = m_rlcBufferReq_1.begin ();
  //       std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator temp;
  //       while (it!=m_rlcBufferReq_1.end ())
  //         {
  //           if (((*it).first.m_rnti == params.m_rnti) && ((*it).first.m_lcId == params.m_logicalChannelIdentity.at (i)))
  //             {
  //               temp = it;
  //               it++;
  //               m_rlcBufferReq_1.erase (temp);
  //             }
  //           else
  //             {
  //               it++;
  //             }
  //         }
  //     }
  //   return;
  // }
  // else if(m_flag == 3)
  // {
  //   NS_LOG_FUNCTION (this);
  // for (uint16_t i = 0; i < params.m_logicalChannelIdentity.size (); i++)
  //   {
  //     std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it = m_rlcBufferReq_1.begin ();
  //     std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator temp;
  //     while (it!=m_rlcBufferReq_1.end ())
  //       {
  //         if (((*it).first.m_rnti == params.m_rnti) && ((*it).first.m_lcId == params.m_logicalChannelIdentity.at (i)))
  //           {
  //             temp = it;
  //             it++;
  //             m_rlcBufferReq_1.erase (temp);
  //           }
  //         else
  //           {
  //             it++;
  //           }
  //       }
  //   }
  // return;
  // }
  // else
  // {
  //   std::cout << "执行1" << std::endl;
    NS_LOG_FUNCTION (this);
  std::vector <uint8_t>::const_iterator it;

  for (it = params.m_logicalChannelIdentity.begin (); it != params.m_logicalChannelIdentity.end (); it++)
    {
      LteFlowId_t flowId = LteFlowId_t (params.m_rnti, *it);

      // find the logical channel with the same Logical Channel Identity in the current list, release it
      if (m_ueLogicalChannelsConfigList.find (flowId)!= m_ueLogicalChannelsConfigList.end ())
        {
          m_ueLogicalChannelsConfigList.erase (flowId);
        }
      else
        {
          NS_FATAL_ERROR ("Logical channels cannot be released because it can not be found in the list of active LCs");
        }
    }
	
  for (uint16_t i = 0; i < params.m_logicalChannelIdentity.size (); i++)
    {
      std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it = m_rlcBufferReq_1.begin ();
      std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator temp;
      while (it!=m_rlcBufferReq_1.end ())
        {
          //std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
          
          if (((*it).first.m_rnti == params.m_rnti) && ((*it).first.m_lcId == params.m_logicalChannelIdentity.at (i)))
            {
              temp = it;
              it++;
              m_rlcBufferReq_1.erase (temp);
            }
          else
            {
              it++;
            }
        }
    }
  return;
  // }
}

void
RrFfMacScheduler::DoCschedUeReleaseReq (const struct FfMacCschedSapProvider::CschedUeReleaseReqParameters& params)
{
  // if(m_flag == 0)
  // {
  //   std::cout << "执行0++++++++++++++++" << std::endl;
  NS_LOG_FUNCTION (this << " Release RNTI " << params.m_rnti);
  
  m_uesTxMode.erase (params.m_rnti);
  m_dlHarqCurrentProcessId.erase (params.m_rnti);
  m_dlHarqProcessesStatus.erase  (params.m_rnti);
  m_dlHarqProcessesTimer.erase (params.m_rnti);
  m_dlHarqProcessesDciBuffer.erase  (params.m_rnti);
  m_dlHarqProcessesRlcPduListBuffer.erase  (params.m_rnti);
  m_ulHarqCurrentProcessId.erase  (params.m_rnti);
  m_ulHarqProcessesStatus.erase  (params.m_rnti);
  m_ulHarqProcessesDciBuffer.erase  (params.m_rnti);
  m_ceBsrRxed.erase (params.m_rnti);
  std::list<FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it = m_rlcBufferReq.begin ();
  while (it != m_rlcBufferReq.end ())
    {
      if ((*it).m_rnti == params.m_rnti)
        {
          NS_LOG_INFO (this << " Erase RNTI " << (*it).m_rnti << " LC " << (uint16_t)(*it).m_logicalChannelIdentity);
          it = m_rlcBufferReq.erase (it);
        }
      else
        {
          it++;
        }
    }
  if (m_nextRntiUl == params.m_rnti)
    {
      m_nextRntiUl = 0;
    }

  if (m_nextRntiDl == params.m_rnti)
    {
      m_nextRntiDl = 0;
    }
    
  // return;
  // }
  // if(m_flag == 2)
  // {
  //   NS_LOG_FUNCTION (this);

  m_uesTxMode.erase (params.m_rnti);
  m_dlHarqCurrentProcessId.erase (params.m_rnti);
  m_dlHarqProcessesStatus.erase  (params.m_rnti);
  m_dlHarqProcessesTimer.erase (params.m_rnti);
  m_dlHarqProcessesDciBuffer.erase  (params.m_rnti);
  m_dlHarqProcessesRlcPduListBuffer.erase  (params.m_rnti);
  m_ulHarqCurrentProcessId.erase  (params.m_rnti);
  m_ulHarqProcessesStatus.erase  (params.m_rnti);
  m_ulHarqProcessesDciBuffer.erase  (params.m_rnti);
  m_flowStatsDl.erase  (params.m_rnti);
  m_flowStatsUl.erase  (params.m_rnti);
  m_ceBsrRxed.erase (params.m_rnti);
  std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it1 = m_rlcBufferReq_1.begin ();
  std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator temp;
  while (it1!=m_rlcBufferReq_1.end ())
    {
      if ((*it1).first.m_rnti == params.m_rnti)
        {
          temp = it1;
          it1++;
          m_rlcBufferReq_1.erase (temp);
        }
      else
        {
          it1++;
        }
    }
  if (m_nextRntiUl == params.m_rnti)
    {
      m_nextRntiUl = 0;
    }

  // return;
  // }
  // else if(m_flag == 3)
  // {
  //   NS_LOG_FUNCTION (this);
  
  m_uesTxMode.erase (params.m_rnti);
  m_dlHarqCurrentProcessId.erase (params.m_rnti);
  m_dlHarqProcessesStatus.erase  (params.m_rnti);
  m_dlHarqProcessesTimer.erase (params.m_rnti);
  m_dlHarqProcessesDciBuffer.erase  (params.m_rnti);
  m_dlHarqProcessesRlcPduListBuffer.erase  (params.m_rnti);
  m_ulHarqCurrentProcessId.erase  (params.m_rnti);
  m_ulHarqProcessesStatus.erase  (params.m_rnti);
  m_ulHarqProcessesDciBuffer.erase  (params.m_rnti);
  m_flowStatsDl.erase  (params.m_rnti);
  m_flowStatsUl.erase  (params.m_rnti);
  m_ceBsrRxed.erase (params.m_rnti);
  std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it2 = m_rlcBufferReq_1.begin ();
  // std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator temp;
  while (it2!=m_rlcBufferReq_1.end ())
    {
      if ((*it2).first.m_rnti == params.m_rnti)
        {
          temp = it2;
          it2++;
          m_rlcBufferReq_1.erase (temp);
        }
      else
        {
          it2++;
        }
    }
  if (m_nextRntiUl == params.m_rnti)
    {
      m_nextRntiUl = 0;
    }

  // return;
  // }
  // else
  // {
  //   std::cout << "执行1" << std::endl;
  //    NS_LOG_FUNCTION (this);

    for (int i=0; i < MAX_LC_LIST; i++)
      {
        LteFlowId_t flowId = LteFlowId_t (params.m_rnti,i);
        // find the logical channel with the same Logical Channel Identity in the current list, release it
        if (m_ueLogicalChannelsConfigList.find (flowId)!= m_ueLogicalChannelsConfigList.end ())
          {
            m_ueLogicalChannelsConfigList.erase (flowId);
          }
      }

    m_uesTxMode.erase (params.m_rnti);
    m_dlHarqCurrentProcessId.erase (params.m_rnti);
    m_dlHarqProcessesStatus.erase  (params.m_rnti);
    m_dlHarqProcessesTimer.erase (params.m_rnti);
    m_dlHarqProcessesDciBuffer.erase  (params.m_rnti);
    m_dlHarqProcessesRlcPduListBuffer.erase  (params.m_rnti);
    m_ulHarqCurrentProcessId.erase  (params.m_rnti);
    m_ulHarqProcessesStatus.erase  (params.m_rnti);
    m_ulHarqProcessesDciBuffer.erase  (params.m_rnti);
    m_flowStatsDl.erase  (params.m_rnti);
    m_flowStatsUl.erase  (params.m_rnti);
    m_ceBsrRxed.erase (params.m_rnti);
    std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it3 = m_rlcBufferReq_1.begin ();
    // std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator temp;
    while (it3!=m_rlcBufferReq_1.end ())
      {
        if ((*it3).first.m_rnti == params.m_rnti)
          {
            temp = it3;
            it3++;
            m_rlcBufferReq_1.erase (temp);
          }
        else
          {
            it3++;
          }
      }
    if (m_nextRntiUl == params.m_rnti)
      {
        m_nextRntiUl = 0;
      }

    return;
  // }
    
}


void
RrFfMacScheduler::DoSchedDlRlcBufferReq (const struct FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& params)
{
  
    // std::cout << "执行0" << std::endl;
  NS_LOG_FUNCTION (this << params.m_rnti << (uint32_t) params.m_logicalChannelIdentity);
  // API generated by RLC for updating RLC parameters on a LC (tx and retx queues)
  std::list<FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it1 = m_rlcBufferReq.begin ();
  bool newLc = true;
  while (it1 != m_rlcBufferReq.end ())
    {
      // remove old entries of this UE-LC
      if (((*it1).m_rnti == params.m_rnti)&&((*it1).m_logicalChannelIdentity == params.m_logicalChannelIdentity))
        {
          it1 = m_rlcBufferReq.erase (it1);
          newLc = false;
        }
      else
        {
          ++it1;
        }
    }
  // add the new parameters
  m_rlcBufferReq.insert (it1, params);
  NS_LOG_INFO (this << " RNTI " << params.m_rnti << " LC " << (uint16_t)params.m_logicalChannelIdentity << " RLC tx size " << params.m_rlcTransmissionQueueHolDelay << " RLC retx size " << params.m_rlcRetransmissionQueueSize << " RLC stat size " <<  params.m_rlcStatusPduSize);
  // initialize statistics of the flow in case of new flows
  if (newLc == true)
    {
      m_p10CqiRxed.insert ( std::pair<uint16_t, uint8_t > (params.m_rnti, 1)); // only codeword 0 at this stage (SISO)
      // initialized to 1 (i.e., the lowest value for transmitting a signal)
      m_p10CqiTimers.insert ( std::pair<uint16_t, uint32_t > (params.m_rnti, m_cqiTimersThreshold));
    }

  
 
    NS_LOG_FUNCTION (this << params.m_rnti << (uint32_t) params.m_logicalChannelIdentity);
  // API generated by RLC for updating RLC parameters on a LC (tx and retx queues)

  std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;

  LteFlowId_t flow (params.m_rnti, params.m_logicalChannelIdentity);

  it =  m_rlcBufferReq_1.find (flow);

  if (it == m_rlcBufferReq_1.end ())
    {
      m_rlcBufferReq_1.insert (std::pair <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters> (flow, params));
    }
  else
    {
      (*it).second = params;
    }

  return;
  

 
}

void
RrFfMacScheduler::DoSchedDlPagingBufferReq (const struct FfMacSchedSapProvider::SchedDlPagingBufferReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("method not implemented");
  return;
}

void
RrFfMacScheduler::DoSchedDlMacBufferReq (const struct FfMacSchedSapProvider::SchedDlMacBufferReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("method not implemented");
  return;
}

int
RrFfMacScheduler::GetRbgSize (int dlbandwidth)
{
  for (int i = 0; i < 4; i++)
    {
      if (dlbandwidth < Type0AllocationRbg[i])
        {
          return (i + 1);
        }
    }

  return (-1);
}

unsigned int
RrFfMacScheduler::LcActivePerFlow (uint16_t rnti)
{
  std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
  unsigned int lcActive = 0;
  for (it = m_rlcBufferReq_1.begin (); it != m_rlcBufferReq_1.end (); it++)
    {
      // std::cout << it->first.m_rnti << "+-+-+-+-++++++++" << std::endl;
      if (((*it).first.m_rnti == rnti) && (((*it).second.m_rlcTransmissionQueueSize > 0)
                                           || ((*it).second.m_rlcRetransmissionQueueSize > 0)
                                           || ((*it).second.m_rlcStatusPduSize > 0) ))
        {
          lcActive++;
        }
      if ((*it).first.m_rnti > rnti)
        {
          break;
        }
    }
  return (lcActive);

}

bool
RrFfMacScheduler::SortRlcBufferReq (FfMacSchedSapProvider::SchedDlRlcBufferReqParameters i,FfMacSchedSapProvider::SchedDlRlcBufferReqParameters j)
{
  return (i.m_rnti < j.m_rnti);
}


uint8_t
RrFfMacScheduler::HarqProcessAvailability (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);

  std::map <uint16_t, uint8_t>::iterator it = m_dlHarqCurrentProcessId.find (rnti);
  // std::cout << "-=-=-=00900" << std::endl;
  // for(std::map <uint16_t, uint8_t>::iterator kkk = m_dlHarqCurrentProcessId.begin(); kkk != m_dlHarqCurrentProcessId.end(); kkk++)
  // {
  //   std::cout << kkk->first << std::endl;
  // }
  //  std::cout << "-=-=-=00900" << std::endl;
  if (it == m_dlHarqCurrentProcessId.end ())
    {
      NS_FATAL_ERROR ("No Process Id found for this RNTI " << rnti);
      // return (false);
    }
  std::map <uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find (rnti);
  if (itStat == m_dlHarqProcessesStatus.end ())
    {
      NS_FATAL_ERROR ("No Process Id Statusfound for this RNTI " << rnti);
      // return (false);
    }
  uint8_t i = (*it).second;
  do
    {
      i = (i + 1) % HARQ_PROC_NUM;
    }
  while ( ((*itStat).second.at (i) != 0)&&(i != (*it).second));
  if ((*itStat).second.at (i) == 0)
    {
      return (true);
    }
  else
    {
      return (false); // return a not valid harq proc id
    }
}



uint8_t
RrFfMacScheduler::UpdateHarqProcessId (uint16_t rnti)
{
  if(m_flag == 0)
  {
    // std::cout << "执行0" << std::endl;
    // std::cout << rnti << std::endl;
  NS_LOG_FUNCTION (this << rnti);


  if (m_harqOn == false)
    {
      return (0);
    }

  std::map <uint16_t, uint8_t>::iterator it = m_dlHarqCurrentProcessId.find (rnti);
  if (it == m_dlHarqCurrentProcessId.end ())
    {
      NS_FATAL_ERROR ("No Process Id found for this RNTI " << rnti);
    }
  std::map <uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find (rnti);
  if (itStat == m_dlHarqProcessesStatus.end ())
    {
      NS_FATAL_ERROR ("No Process Id Statusfound for this RNTI " << rnti);
    }
  uint8_t i = (*it).second;
  do
    {
      i = (i + 1) % HARQ_PROC_NUM;
    }
  while ( ((*itStat).second.at (i) != 0)&&(i != (*it).second));
  if ((*itStat).second.at (i) == 0)
    {
      (*it).second = i;
      (*itStat).second.at (i) = 1;
    }
  else
    {
      return (9); // return a not valid harq proc id
    }

  return ((*it).second);
  }
  else if(m_flag == 2)
  {
    NS_LOG_FUNCTION (this << rnti);

  if (m_harqOn == false)
    {
      return (0);
    }


  std::map <uint16_t, uint8_t>::iterator it = m_dlHarqCurrentProcessId.find (rnti);
  if (it == m_dlHarqCurrentProcessId.end ())
    {
      NS_FATAL_ERROR ("No Process Id found for this RNTI " << rnti);
    }
  std::map <uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find (rnti);
  if (itStat == m_dlHarqProcessesStatus.end ())
    {
      NS_FATAL_ERROR ("No Process Id Statusfound for this RNTI " << rnti);
    }
  uint8_t i = (*it).second;
  do
    {
      i = (i + 1) % HARQ_PROC_NUM;
    }
  while ( ((*itStat).second.at (i) != 0)&&(i != (*it).second));
  if ((*itStat).second.at (i) == 0)
    {
      (*it).second = i;
      (*itStat).second.at (i) = 1;
    }
  else
    {
      return (9); // return a not valid harq proc id
      // NS_FATAL_ERROR ("No HARQ process available for RNTI " << rnti << " check before update with HarqProcessAvailability");
    }

  return ((*it).second);
  }
  else if(m_flag == 3)
  {
    NS_LOG_FUNCTION (this << rnti);

  if (m_harqOn == false)
    {
      return (0);
    }


  std::map <uint16_t, uint8_t>::iterator it = m_dlHarqCurrentProcessId.find (rnti);
  if (it == m_dlHarqCurrentProcessId.end ())
    {
      NS_FATAL_ERROR ("No Process Id found for this RNTI " << rnti);
    }
  std::map <uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find (rnti);
  if (itStat == m_dlHarqProcessesStatus.end ())
    {
      NS_FATAL_ERROR ("No Process Id Statusfound for this RNTI " << rnti);
    }
  uint8_t i = (*it).second;
  do
    {
      i = (i + 1) % HARQ_PROC_NUM;
    }
  while ( ((*itStat).second.at (i) != 0)&&(i != (*it).second));
  if ((*itStat).second.at (i) == 0)
    {
      (*it).second = i;
      (*itStat).second.at (i) = 1;
    }
  else
    {
      return (9); // return a not valid harq proc id
      // NS_FATAL_ERROR ("No HARQ process available for RNTI " << rnti << " check before update with HarqProcessAvailability");
    }

  return ((*it).second);
  }
  else
  {
    // std::cout << "执行1" << std::endl;
    // std::cout << rnti << std::endl;
    NS_LOG_FUNCTION (this << rnti);

  if (m_harqOn == false)
    {
      return (0);
    }

  
  std::map <uint16_t, uint8_t>::iterator it = m_dlHarqCurrentProcessId.find (rnti);
  // for(std::map <uint16_t, uint8_t>::iterator iit = m_dlHarqCurrentProcessId.begin(); iit != m_dlHarqCurrentProcessId.end(); iit++)
  // {
  //   std::cout << iit ->first << std::endl;
  // }
  // std::cout << "875667435469+8536354" << std::endl;
  if (it == m_dlHarqCurrentProcessId.end ())
    {
      NS_FATAL_ERROR ("No Process Id found for this RNTI " << rnti);
    }
  std::map <uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find (rnti);
  if (itStat == m_dlHarqProcessesStatus.end ())
    {
      NS_FATAL_ERROR ("No Process Id Statusfound for this RNTI " << rnti);
    }
  uint8_t i = (*it).second;
  do
    {
      i = (i + 1) % HARQ_PROC_NUM;
    }
  while ( ((*itStat).second.at (i) != 0)&&(i != (*it).second));
  if ((*itStat).second.at (i) == 0)
    {
      (*it).second = i;
      (*itStat).second.at (i) = 1;
    }
  else
    {
      return (9); // return a not valid harq proc id
      // NS_FATAL_ERROR ("No HARQ process available for RNTI " << rnti << " check before update with HarqProcessAvailability");
    }

  return ((*it).second);
  }
}


void
RrFfMacScheduler::RefreshHarqProcesses ()
{
  NS_LOG_FUNCTION (this);

  std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itTimers;
  for (itTimers = m_dlHarqProcessesTimer.begin (); itTimers != m_dlHarqProcessesTimer.end (); itTimers ++)
    {
      for (uint16_t i = 0; i < HARQ_PROC_NUM; i++)
        {
          if ((*itTimers).second.at (i) == HARQ_DL_TIMEOUT)
            {
              // reset HARQ process

              NS_LOG_INFO (this << " Reset HARQ proc " << i << " for RNTI " << (*itTimers).first);
              std::map <uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find ((*itTimers).first);
              if (itStat == m_dlHarqProcessesStatus.end ())
                {
                  NS_FATAL_ERROR ("No Process Id Status found for this RNTI " << (*itTimers).first);
                }
              (*itStat).second.at (i) = 0;
              (*itTimers).second.at (i) = 0;
            }
          else
            {
              (*itTimers).second.at (i)++;
            }
        }
    }

}



void
RrFfMacScheduler::DoSchedDlTriggerReq (const struct FfMacSchedSapProvider::SchedDlTriggerReqParameters& params)
{
  if(m_flag == 0)
  {
    // std::cout << "执行0" <<std::endl;
  NS_LOG_FUNCTION (this << " DL Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
  // API generated by RLC for triggering the scheduling of a DL subframe

  RefreshDlCqiMaps ();
  int rbgSize = GetRbgSize (m_cschedCellConfig.m_dlBandwidth);
  int rbgNum = m_cschedCellConfig.m_dlBandwidth / rbgSize;
  FfMacSchedSapUser::SchedDlConfigIndParameters ret;

  // Generate RBGs map
  std::vector <bool> rbgMap;
  uint16_t rbgAllocatedNum = 0;
  std::set <uint16_t> rntiAllocated;
  rbgMap.resize (m_cschedCellConfig.m_dlBandwidth / rbgSize, false);

  //   update UL HARQ proc id
  std::map <uint16_t, uint8_t>::iterator itProcId;
  for (itProcId = m_ulHarqCurrentProcessId.begin (); itProcId != m_ulHarqCurrentProcessId.end (); itProcId++)
    {
      (*itProcId).second = ((*itProcId).second + 1) % HARQ_PROC_NUM;
    }

  // RACH Allocation
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  uint16_t rbStart = 0;
  std::vector <struct RachListElement_s>::iterator itRach;
  for (itRach = m_rachList.begin (); itRach != m_rachList.end (); itRach++)
    {
      NS_ASSERT_MSG (m_amc->GetUlTbSizeFromMcs (m_ulGrantMcs, m_cschedCellConfig.m_ulBandwidth) > (*itRach).m_estimatedSize, " Default UL Grant MCS does not allow to send RACH messages");
      BuildRarListElement_s newRar;
      newRar.m_rnti = (*itRach).m_rnti;
      // DL-RACH Allocation
      // Ideal: no needs of configuring m_dci
      // UL-RACH Allocation
      newRar.m_grant.m_rnti = newRar.m_rnti;
      newRar.m_grant.m_mcs = m_ulGrantMcs;
      uint16_t rbLen = 1;
      uint16_t tbSizeBits = 0;
      // find lowest TB size that fits UL grant estimated size
      while ((tbSizeBits < (*itRach).m_estimatedSize) && (rbStart + rbLen < m_cschedCellConfig.m_ulBandwidth))
        {
          rbLen++;
          tbSizeBits = m_amc->GetUlTbSizeFromMcs (m_ulGrantMcs, rbLen);
        }
      if (tbSizeBits < (*itRach).m_estimatedSize)
        {
          // no more allocation space: finish allocation
          break;
        }
      newRar.m_grant.m_rbStart = rbStart;
      newRar.m_grant.m_rbLen = rbLen;
      newRar.m_grant.m_tbSize = tbSizeBits / 8;
      newRar.m_grant.m_hopping = false;
      newRar.m_grant.m_tpc = 0;
      newRar.m_grant.m_cqiRequest = false;
      newRar.m_grant.m_ulDelay = false;
      NS_LOG_INFO (this << " UL grant allocated to RNTI " << (*itRach).m_rnti << " rbStart " << rbStart << " rbLen " << rbLen << " MCS " << (uint16_t) m_ulGrantMcs << " tbSize " << newRar.m_grant.m_tbSize);
      for (uint16_t i = rbStart; i < rbStart + rbLen; i++)
        {
          m_rachAllocationMap.at (i) = (*itRach).m_rnti;
        }

      if (m_harqOn == true)
        {
          // generate UL-DCI for HARQ retransmissions
          UlDciListElement_s uldci;
          uldci.m_rnti = newRar.m_rnti;
          uldci.m_rbLen = rbLen;
          uldci.m_rbStart = rbStart;
          uldci.m_mcs = m_ulGrantMcs;
          uldci.m_tbSize = tbSizeBits / 8;
          uldci.m_ndi = 1;
          uldci.m_cceIndex = 0;
          uldci.m_aggrLevel = 1;
          uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
          uldci.m_hopping = false;
          uldci.m_n2Dmrs = 0;
          uldci.m_tpc = 0; // no power control
          uldci.m_cqiRequest = false; // only period CQI at this stage
          uldci.m_ulIndex = 0; // TDD parameter
          uldci.m_dai = 1; // TDD parameter
          uldci.m_freqHopping = 0;
          uldci.m_pdcchPowerOffset = 0; // not used

          uint8_t harqId = 0;
          std::map <uint16_t, uint8_t>::iterator itProcId;
          itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
          if (itProcId == m_ulHarqCurrentProcessId.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
            }
          harqId = (*itProcId).second;
          std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
          if (itDci == m_ulHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
            }
          (*itDci).second.at (harqId) = uldci;
        }

      rbStart = rbStart + rbLen;
      ret.m_buildRarList.push_back (newRar);
    }
  m_rachList.clear ();

  // Process DL HARQ feedback
  RefreshHarqProcesses ();
  // retrieve past HARQ retx buffered
  if (m_dlInfoListBuffered.size () > 0)
    {
      if (params.m_dlInfoList.size () > 0)
        {
          NS_LOG_INFO (this << " Received DL-HARQ feedback");
          m_dlInfoListBuffered.insert (m_dlInfoListBuffered.end (), params.m_dlInfoList.begin (), params.m_dlInfoList.end ());
        }
    }
  else
    {
      if (params.m_dlInfoList.size () > 0)
        {
          m_dlInfoListBuffered = params.m_dlInfoList;
        }
    }
  if (m_harqOn == false)
    {
      // Ignore HARQ feedback
      m_dlInfoListBuffered.clear ();
    }
  std::vector <struct DlInfoListElement_s> dlInfoListUntxed;
  for (uint16_t i = 0; i < m_dlInfoListBuffered.size (); i++)
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find (m_dlInfoListBuffered.at (i).m_rnti);
      if (itRnti != rntiAllocated.end ())
        {
          // RNTI already allocated for retx
          continue;
        }
      uint8_t nLayers = m_dlInfoListBuffered.at (i).m_harqStatus.size ();
      std::vector <bool> retx;
      NS_LOG_INFO (this << " Processing DLHARQ feedback");
      if (nLayers == 1)
        {
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (0) == DlInfoListElement_s::NACK);
          retx.push_back (false);
        }
      else
        {
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (0) == DlInfoListElement_s::NACK);
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (1) == DlInfoListElement_s::NACK);
        }
      if (retx.at (0) || retx.at (1))
        {
          // retrieve HARQ process information
          uint16_t rnti = m_dlInfoListBuffered.at (i).m_rnti;
          uint8_t harqId = m_dlInfoListBuffered.at (i).m_harqProcessId;
          NS_LOG_INFO (this << " HARQ retx RNTI " << rnti << " harqId " << (uint16_t)harqId);
          std::map <uint16_t, DlHarqProcessesDciBuffer_t>::iterator itHarq = m_dlHarqProcessesDciBuffer.find (rnti);
          if (itHarq == m_dlHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << rnti);
            }

          DlDciListElement_s dci = (*itHarq).second.at (harqId);
          int rv = 0;
          if (dci.m_rv.size () == 1)
            {
              rv = dci.m_rv.at (0);
            }
          else
            {
              rv = (dci.m_rv.at (0) > dci.m_rv.at (1) ? dci.m_rv.at (0) : dci.m_rv.at (1));
            }

          if (rv == 3)
            {
              // maximum number of retx reached -> drop process
              NS_LOG_INFO ("Max number of retransmissions reached -> drop process");
              std::map <uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find (rnti);
              if (it == m_dlHarqProcessesStatus.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << m_dlInfoListBuffered.at (i).m_rnti);
                }
              (*it).second.at (harqId) = 0;
              std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (rnti);
              if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
                {
                  NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at (i).m_rnti);
                }
              for (uint16_t k = 0; k < (*itRlcPdu).second.size (); k++)
                {
                  (*itRlcPdu).second.at (k).at (harqId).clear ();
                }
              continue;
            }
          // check the feasibility of retransmitting on the same RBGs
          // translate the DCI to Spectrum framework
          std::vector <int> dciRbg;
          uint32_t mask = 0x1;
          NS_LOG_INFO ("Original RBGs " << dci.m_rbBitmap << " rnti " << dci.m_rnti);
          for (int j = 0; j < 32; j++)
            {
              if (((dci.m_rbBitmap & mask) >> j) == 1)
                {
                  dciRbg.push_back (j);
                  NS_LOG_INFO ("\t" << j);
                }
              mask = (mask << 1);
            }
          bool free = true;
          for (uint8_t j = 0; j < dciRbg.size (); j++)
            {
              if (rbgMap.at (dciRbg.at (j)) == true)
                {
                  free = false;
                  break;
                }
            }
          if (free)
            {
              // use the same RBGs for the retx
              // reserve RBGs
              for (uint8_t j = 0; j < dciRbg.size (); j++)
                {
                  rbgMap.at (dciRbg.at (j)) = true;
                  NS_LOG_INFO ("RBG " << dciRbg.at (j) << " assigned");
                  rbgAllocatedNum++;
                }

              NS_LOG_INFO (this << " Send retx in the same RBGs");
            }
          else
            {
              // find RBGs for sending HARQ retx
              uint8_t j = 0;
              uint8_t rbgId = (dciRbg.at (dciRbg.size () - 1) + 1) % rbgNum;
              uint8_t startRbg = dciRbg.at (dciRbg.size () - 1);
              std::vector <bool> rbgMapCopy = rbgMap;
              while ((j < dciRbg.size ())&&(startRbg != rbgId))
                {
                  if (rbgMapCopy.at (rbgId) == false)
                    {
                      rbgMapCopy.at (rbgId) = true;
                      dciRbg.at (j) = rbgId;
                      j++;
                    }
                  rbgId = (rbgId + 1) % rbgNum;
                }
              if (j == dciRbg.size ())
                {
                  // find new RBGs -> update DCI map
                  uint32_t rbgMask = 0;
                  for (uint16_t k = 0; k < dciRbg.size (); k++)
                    {
                      rbgMask = rbgMask + (0x1 << dciRbg.at (k));
                      NS_LOG_INFO (this << " New allocated RBG " << dciRbg.at (k));
                      rbgAllocatedNum++;
                    }
                  dci.m_rbBitmap = rbgMask;
                  rbgMap = rbgMapCopy;
                }
              else
                {
                  // HARQ retx cannot be performed on this TTI -> store it
                  dlInfoListUntxed.push_back (m_dlInfoListBuffered.at (i));
                  NS_LOG_INFO (this << " No resource for this retx -> buffer it");
                }
            }
          // retrieve RLC PDU list for retx TBsize and update DCI
          BuildDataListElement_s newEl;
          std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (rnti);
          if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << rnti);
            }
          for (uint8_t j = 0; j < nLayers; j++)
            {
              if (retx.at (j))
                {
                  if (j >= dci.m_ndi.size ())
                    {
                      // for avoiding errors in MIMO transient phases
                      dci.m_ndi.push_back (0);
                      dci.m_rv.push_back (0);
                      dci.m_mcs.push_back (0);
                      dci.m_tbsSize.push_back (0);
                      NS_LOG_INFO (this << " layer " << (uint16_t)j << " no txed (MIMO transition)");

                    }
                  else
                    {
                      dci.m_ndi.at (j) = 0;
                      dci.m_rv.at (j)++;
                      (*itHarq).second.at (harqId).m_rv.at (j)++;
                      NS_LOG_INFO (this << " layer " << (uint16_t)j << " RV " << (uint16_t)dci.m_rv.at (j));
                    }
                }
              else
                {
                  // empty TB of layer j
                  dci.m_ndi.at (j) = 0;
                  dci.m_rv.at (j) = 0;
                  dci.m_mcs.at (j) = 0;
                  dci.m_tbsSize.at (j) = 0;
                  NS_LOG_INFO (this << " layer " << (uint16_t)j << " no retx");
                }
            }

          for (uint16_t k = 0; k < (*itRlcPdu).second.at (0).at (dci.m_harqProcess).size (); k++)
            {
              std::vector <struct RlcPduListElement_s> rlcPduListPerLc;
              for (uint8_t j = 0; j < nLayers; j++)
                {
                  if (retx.at (j))
                    {
                      if (j < dci.m_ndi.size ())
                        {
                          NS_LOG_INFO (" layer " << (uint16_t)j << " tb size " << dci.m_tbsSize.at (j));
                          rlcPduListPerLc.push_back ((*itRlcPdu).second.at (j).at (dci.m_harqProcess).at (k));
                        }
                    }
                  else
                    { // if no retx needed on layer j, push an RlcPduListElement_s object with m_size=0 to keep the size of rlcPduListPerLc vector = 2 in case of MIMO
                      NS_LOG_INFO (" layer " << (uint16_t)j << " tb size "<<dci.m_tbsSize.at (j));
                      RlcPduListElement_s emptyElement;
                      emptyElement.m_logicalChannelIdentity = (*itRlcPdu).second.at (j).at (dci.m_harqProcess).at (k).m_logicalChannelIdentity;
                      emptyElement.m_size = 0;
                      rlcPduListPerLc.push_back (emptyElement);
                    }
                }

              if (rlcPduListPerLc.size () > 0)
                {
                  newEl.m_rlcPduList.push_back (rlcPduListPerLc);
                }
            }
          newEl.m_rnti = rnti;
          newEl.m_dci = dci;
          (*itHarq).second.at (harqId).m_rv = dci.m_rv;
          // refresh timer
          std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer = m_dlHarqProcessesTimer.find (rnti);
          if (itHarqTimer== m_dlHarqProcessesTimer.end ())
            {
              NS_FATAL_ERROR ("Unable to find HARQ timer for RNTI " << (uint16_t)rnti);
            }
          (*itHarqTimer).second.at (harqId) = 0;
          ret.m_buildDataList.push_back (newEl);
          rntiAllocated.insert (rnti);
        }
      else
        {
          // update HARQ process status
          NS_LOG_INFO (this << " HARQ ACK UE " << m_dlInfoListBuffered.at (i).m_rnti);
          std::map <uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find (m_dlInfoListBuffered.at (i).m_rnti);
          if (it == m_dlHarqProcessesStatus.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << m_dlInfoListBuffered.at (i).m_rnti);
            }
          (*it).second.at (m_dlInfoListBuffered.at (i).m_harqProcessId) = 0;
          std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (m_dlInfoListBuffered.at (i).m_rnti);
          if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at (i).m_rnti);
            }
          for (uint16_t k = 0; k < (*itRlcPdu).second.size (); k++)
            {
              (*itRlcPdu).second.at (k).at (m_dlInfoListBuffered.at (i).m_harqProcessId).clear ();
            }
        }
    }
  m_dlInfoListBuffered.clear ();
  m_dlInfoListBuffered = dlInfoListUntxed;

  if (rbgAllocatedNum == rbgNum)
    {
      // all the RBGs are already allocated -> exit
      if ((ret.m_buildDataList.size () > 0) || (ret.m_buildRarList.size () > 0))
        {
          m_schedSapUser->SchedDlConfigInd (ret);
        }
      return;
    }

  // Get the actual active flows (queue!=0)
  std::list<FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
  m_rlcBufferReq.sort (SortRlcBufferReq);
  int nflows = 0;
  int nTbs = 0;
  std::map <uint16_t,uint8_t> lcActivesPerRnti; // tracks how many active LCs per RNTI there are
  std::map <uint16_t,uint8_t>::iterator itLcRnti;
  for (it = m_rlcBufferReq.begin (); it != m_rlcBufferReq.end (); it++)
    {
      // remove old entries of this UE-LC
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).m_rnti);
      if ( (((*it).m_rlcTransmissionQueueSize > 0)
            || ((*it).m_rlcRetransmissionQueueSize > 0)
            || ((*it).m_rlcStatusPduSize > 0))
           && (itRnti == rntiAllocated.end ())  // UE must not be allocated for HARQ retx
           && (HarqProcessAvailability ((*it).m_rnti))  ) // UE needs HARQ proc free

        {
          NS_LOG_LOGIC (this << " User " << (*it).m_rnti << " LC " << (uint16_t)(*it).m_logicalChannelIdentity << " is active, status  " << (*it).m_rlcStatusPduSize << " retx " << (*it).m_rlcRetransmissionQueueSize << " tx " << (*it).m_rlcTransmissionQueueSize);
          std::map <uint16_t,uint8_t>::iterator itCqi = m_p10CqiRxed.find ((*it).m_rnti);
          uint8_t cqi = 0;
          if (itCqi != m_p10CqiRxed.end ())
            {
              cqi = (*itCqi).second;
            }
          else
            {
              cqi = 1; // lowest value for trying a transmission
            }
          if (cqi != 0)
            {
              // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
              nflows++;
              itLcRnti = lcActivesPerRnti.find ((*it).m_rnti);
              if (itLcRnti != lcActivesPerRnti.end ())
                {
                  (*itLcRnti).second++;
                }
              else
                {
                  lcActivesPerRnti.insert (std::pair<uint16_t, uint8_t > ((*it).m_rnti, 1));
                  nTbs++;
                }

            }
        }
    }

  if (nflows == 0)
    {
      if ((ret.m_buildDataList.size () > 0) || (ret.m_buildRarList.size () > 0))
        {
          m_schedSapUser->SchedDlConfigInd (ret);
        }
      return;
    }
  // Divide the resource equally among the active users according to
  // Resource allocation type 0 (see sec 7.1.6.1 of 36.213)

  int rbgPerTb = (nTbs > 0) ? ((rbgNum - rbgAllocatedNum) / nTbs) : INT_MAX;
  
  NS_LOG_INFO (this << " Flows to be transmitted " << nflows << " rbgPerTb " << rbgPerTb);
  if (rbgPerTb == 0)
    {
      rbgPerTb = 1;                // at least 1 rbg per TB (till available resource)
    }
  int rbgAllocated = 0;

  // round robin assignment to all UEs registered starting from the subsequent of the one
  // served last scheduling trigger event
  if (m_nextRntiDl != 0)
    {
      NS_LOG_DEBUG ("Start from the successive of " << (uint16_t) m_nextRntiDl);
      for (it = m_rlcBufferReq.begin (); it != m_rlcBufferReq.end (); it++)
        {
          if ((*it).m_rnti == m_nextRntiDl)
            {
              // select the next RNTI to starting
              it++;
              if (it == m_rlcBufferReq.end ())
              {
                it = m_rlcBufferReq.begin ();
              }
              m_nextRntiDl = (*it).m_rnti;
              break;
            }
        }

      if (it == m_rlcBufferReq.end ())
        {
          NS_LOG_ERROR (this << " no user found");
        }
    }
  else
    {
      it = m_rlcBufferReq.begin ();
      m_nextRntiDl = (*it).m_rnti;
    }
  std::map <uint16_t,uint8_t>::iterator itTxMode;
  do
    {
      itLcRnti = lcActivesPerRnti.find ((*it).m_rnti);
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).m_rnti);
      if ((itLcRnti == lcActivesPerRnti.end ())||(itRnti != rntiAllocated.end ()))
        {
          // skip this RNTI (no active queue or yet allocated for HARQ)
          uint16_t rntiDiscared = (*it).m_rnti;
          while (it != m_rlcBufferReq.end ())
            {
              if ((*it).m_rnti != rntiDiscared)
                {
                  break;
                }
              it++;
            }
          if (it == m_rlcBufferReq.end ())
            {
              // restart from the first
              it = m_rlcBufferReq.begin ();
            }
          continue;
        }
      itTxMode = m_uesTxMode.find ((*it).m_rnti);
      if (itTxMode == m_uesTxMode.end ())
        {
          NS_FATAL_ERROR ("No Transmission Mode info on user " << (*it).m_rnti);
        }
      int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);
      int lcNum = (*itLcRnti).second;
      // create new BuildDataListElement_s for this RNTI
      BuildDataListElement_s newEl;
      newEl.m_rnti = (*it).m_rnti;
      // create the DlDciListElement_s
      DlDciListElement_s newDci;
      newDci.m_rnti = (*it).m_rnti;
      newDci.m_harqProcess = UpdateHarqProcessId ((*it).m_rnti);
      newDci.m_resAlloc = 0;
      newDci.m_rbBitmap = 0;
      std::map <uint16_t,uint8_t>::iterator itCqi = m_p10CqiRxed.find (newEl.m_rnti);
      for (uint8_t i = 0; i < nLayer; i++)
        {
          if (itCqi == m_p10CqiRxed.end ())
            {
              newDci.m_mcs.push_back (0); // no info on this user -> lowest MCS
            }
          else
            {
              newDci.m_mcs.push_back ( m_amc->GetMcsFromCqi ((*itCqi).second) );
            }
        }
      int tbSize = (m_amc->GetDlTbSizeFromMcs (newDci.m_mcs.at (0), rbgPerTb * rbgSize) / 8);
      uint16_t rlcPduSize = tbSize / lcNum;
      while ((*it).m_rnti == newEl.m_rnti)
        {
          if ( ((*it).m_rlcTransmissionQueueSize > 0)
               || ((*it).m_rlcRetransmissionQueueSize > 0)
               || ((*it).m_rlcStatusPduSize > 0) )
            {
              std::vector <struct RlcPduListElement_s> newRlcPduLe;
              for (uint8_t j = 0; j < nLayer; j++)
                {
                  RlcPduListElement_s newRlcEl;
                  newRlcEl.m_logicalChannelIdentity = (*it).m_logicalChannelIdentity;
                  NS_LOG_INFO (this << "LCID " << (uint32_t) newRlcEl.m_logicalChannelIdentity << " size " << rlcPduSize << " ID " << (*it).m_rnti << " layer " << (uint16_t)j);
                  newRlcEl.m_size = rlcPduSize;
                  UpdateDlRlcBufferInfo ((*it).m_rnti, newRlcEl.m_logicalChannelIdentity, rlcPduSize);
                  newRlcPduLe.push_back (newRlcEl);

                  if (m_harqOn == true)
                    {
                      // store RLC PDU list for HARQ
                      std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find ((*it).m_rnti);
                      if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
                        {
                          NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << (*it).m_rnti);
                        }
                      (*itRlcPdu).second.at (j).at (newDci.m_harqProcess).push_back (newRlcEl);
                    }

                }
              newEl.m_rlcPduList.push_back (newRlcPduLe);
              lcNum--;
            }
          it++;
          if (it == m_rlcBufferReq.end ())
            {
              // restart from the first
              it = m_rlcBufferReq.begin ();
              break;
            }
        }
      uint32_t rbgMask = 0;
      uint16_t i = 0;
      NS_LOG_INFO (this << " DL - Allocate user " << newEl.m_rnti << " LCs " << (uint16_t)(*itLcRnti).second << " bytes " << tbSize << " mcs " << (uint16_t) newDci.m_mcs.at (0) << " harqId " << (uint16_t)newDci.m_harqProcess <<  " layers " << nLayer);
      NS_LOG_INFO ("RBG:");
      while (i < rbgPerTb)
        {
          if (rbgMap.at (rbgAllocated) == false)
            {
              rbgMask = rbgMask + (0x1 << rbgAllocated);
              NS_LOG_INFO ("\t " << rbgAllocated);
              i++;
              rbgMap.at (rbgAllocated) = true;
              rbgAllocatedNum++;
            }
          rbgAllocated++;
        }
      newDci.m_rbBitmap = rbgMask; // (32 bit bitmap see 7.1.6 of 36.213)

      for (int i = 0; i < nLayer; i++)
        {
          newDci.m_tbsSize.push_back (tbSize);
          newDci.m_ndi.push_back (1);
          newDci.m_rv.push_back (0);
        }

      newDci.m_tpc = 1; //1 is mapped to 0 in Accumulated Mode and to -1 in Absolute Mode

      newEl.m_dci = newDci;
      if (m_harqOn == true)
        {
          // store DCI for HARQ
          std::map <uint16_t, DlHarqProcessesDciBuffer_t>::iterator itDci = m_dlHarqProcessesDciBuffer.find (newEl.m_rnti);
          if (itDci == m_dlHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in DCI HARQ buffer for RNTI " << newEl.m_rnti);
            }
          (*itDci).second.at (newDci.m_harqProcess) = newDci;
          // refresh timer
          std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer =  m_dlHarqProcessesTimer.find (newEl.m_rnti);
          if (itHarqTimer== m_dlHarqProcessesTimer.end ())
            {
              NS_FATAL_ERROR ("Unable to find HARQ timer for RNTI " << (uint16_t)newEl.m_rnti);
            }
          (*itHarqTimer).second.at (newDci.m_harqProcess) = 0;
        }
      // ...more parameters -> ignored in this version

      ret.m_buildDataList.push_back (newEl);
      if (rbgAllocatedNum == rbgNum)
        {
          m_nextRntiDl = newEl.m_rnti; // store last RNTI served
          break;                       // no more RGB to be allocated
        }
    }
  while ((*it).m_rnti != m_nextRntiDl);

  ret.m_nrOfPdcchOfdmSymbols = 1;   /// \todo check correct value according the DCIs txed  

  m_schedSapUser->SchedDlConfigInd (ret);
  return;
  }
  else if(m_flag == 2)
  {
    // std::cout << "执行2" <<std::endl;
    NS_LOG_FUNCTION (this << " Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
  // API generated by RLC for triggering the scheduling of a DL subframe


  // evaluate the relative channel quality indicator for each UE per each RBG
  // (since we are using allocation type 0 the small unit of allocation is RBG)
  // Resource allocation type 0 (see sec 7.1.6.1 of 36.213)

  RefreshDlCqiMaps ();

  int rbgSize = GetRbgSize (m_cschedCellConfig.m_dlBandwidth);
  int rbgNum = m_cschedCellConfig.m_dlBandwidth / rbgSize;
  std::map <uint16_t, std::vector <uint16_t> > allocationMap; // RBs map per RNTI
  std::vector <bool> rbgMap;  // global RBGs map
  uint16_t rbgAllocatedNum = 0;
  std::set <uint16_t> rntiAllocated;
  rbgMap.resize (m_cschedCellConfig.m_dlBandwidth / rbgSize, false);
  FfMacSchedSapUser::SchedDlConfigIndParameters ret;


  //   update UL HARQ proc id
  std::map <uint16_t, uint8_t>::iterator itProcId;
  for (itProcId = m_ulHarqCurrentProcessId.begin (); itProcId != m_ulHarqCurrentProcessId.end (); itProcId++)
    {
      (*itProcId).second = ((*itProcId).second + 1) % HARQ_PROC_NUM;
    }

  // RACH Allocation
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  uint16_t rbStart = 0;
  std::vector <struct RachListElement_s>::iterator itRach;
  for (itRach = m_rachList.begin (); itRach != m_rachList.end (); itRach++)
    {
      NS_ASSERT_MSG (m_amc->GetUlTbSizeFromMcs (m_ulGrantMcs, m_cschedCellConfig.m_ulBandwidth) > (*itRach).m_estimatedSize, " Default UL Grant MCS does not allow to send RACH messages");
      BuildRarListElement_s newRar;
      newRar.m_rnti = (*itRach).m_rnti;
      // DL-RACH Allocation
      // Ideal: no needs of configuring m_dci
      // UL-RACH Allocation
      newRar.m_grant.m_rnti = newRar.m_rnti;
      newRar.m_grant.m_mcs = m_ulGrantMcs;
      uint16_t rbLen = 1;
      uint16_t tbSizeBits = 0;
      // find lowest TB size that fits UL grant estimated size
      while ((tbSizeBits < (*itRach).m_estimatedSize) && (rbStart + rbLen < m_cschedCellConfig.m_ulBandwidth))
        {
          rbLen++;
          tbSizeBits = m_amc->GetUlTbSizeFromMcs (m_ulGrantMcs, rbLen);
        }
      if (tbSizeBits < (*itRach).m_estimatedSize)
        {
          // no more allocation space: finish allocation
          break;
        }
      newRar.m_grant.m_rbStart = rbStart;
      newRar.m_grant.m_rbLen = rbLen;
      newRar.m_grant.m_tbSize = tbSizeBits / 8;
      newRar.m_grant.m_hopping = false;
      newRar.m_grant.m_tpc = 0;
      newRar.m_grant.m_cqiRequest = false;
      newRar.m_grant.m_ulDelay = false;
      NS_LOG_INFO (this << " UL grant allocated to RNTI " << (*itRach).m_rnti << " rbStart " << rbStart << " rbLen " << rbLen << " MCS " << m_ulGrantMcs << " tbSize " << newRar.m_grant.m_tbSize);
      for (uint16_t i = rbStart; i < rbStart + rbLen; i++)
        {
          m_rachAllocationMap.at (i) = (*itRach).m_rnti;
        }

      if (m_harqOn == true)
        {
          // generate UL-DCI for HARQ retransmissions
          UlDciListElement_s uldci;
          uldci.m_rnti = newRar.m_rnti;
          uldci.m_rbLen = rbLen;
          uldci.m_rbStart = rbStart;
          uldci.m_mcs = m_ulGrantMcs;
          uldci.m_tbSize = tbSizeBits / 8;
          uldci.m_ndi = 1;
          uldci.m_cceIndex = 0;
          uldci.m_aggrLevel = 1;
          uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
          uldci.m_hopping = false;
          uldci.m_n2Dmrs = 0;
          uldci.m_tpc = 0; // no power control
          uldci.m_cqiRequest = false; // only period CQI at this stage
          uldci.m_ulIndex = 0; // TDD parameter
          uldci.m_dai = 1; // TDD parameter
          uldci.m_freqHopping = 0;
          uldci.m_pdcchPowerOffset = 0; // not used

          uint8_t harqId = 0;
          std::map <uint16_t, uint8_t>::iterator itProcId;
          itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
          if (itProcId == m_ulHarqCurrentProcessId.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
            }
          harqId = (*itProcId).second;
          std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
          if (itDci == m_ulHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
            }
          (*itDci).second.at (harqId) = uldci;
        }

      rbStart = rbStart + rbLen;
      ret.m_buildRarList.push_back (newRar);
    }
  m_rachList.clear ();


  // Process DL HARQ feedback
  RefreshHarqProcesses ();
  // retrieve past HARQ retx buffered
  if (m_dlInfoListBuffered.size () > 0)
    {
      if (params.m_dlInfoList.size () > 0)
        {
          NS_LOG_INFO (this << " Received DL-HARQ feedback");
          m_dlInfoListBuffered.insert (m_dlInfoListBuffered.end (), params.m_dlInfoList.begin (), params.m_dlInfoList.end ());
        }
    }
  else
    {
      if (params.m_dlInfoList.size () > 0)
        {
          m_dlInfoListBuffered = params.m_dlInfoList;
        }
    }
  if (m_harqOn == false)
    {
      // Ignore HARQ feedback
      m_dlInfoListBuffered.clear ();
    }
  std::vector <struct DlInfoListElement_s> dlInfoListUntxed;
  for (uint16_t i = 0; i < m_dlInfoListBuffered.size (); i++)
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find (m_dlInfoListBuffered.at (i).m_rnti);
      if (itRnti != rntiAllocated.end ())
        {
          // RNTI already allocated for retx
          continue;
        }
      uint8_t nLayers = m_dlInfoListBuffered.at (i).m_harqStatus.size ();
      std::vector <bool> retx;
      NS_LOG_INFO (this << " Processing DLHARQ feedback");
      if (nLayers == 1)
        {
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (0) == DlInfoListElement_s::NACK);
          retx.push_back (false);
        }
      else
        {
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (0) == DlInfoListElement_s::NACK);
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (1) == DlInfoListElement_s::NACK);
        }
      if (retx.at (0) || retx.at (1))
        {
          // retrieve HARQ process information
          uint16_t rnti = m_dlInfoListBuffered.at (i).m_rnti;
          uint8_t harqId = m_dlInfoListBuffered.at (i).m_harqProcessId;
          NS_LOG_INFO (this << " HARQ retx RNTI " << rnti << " harqId " << (uint16_t)harqId);
          std::map <uint16_t, DlHarqProcessesDciBuffer_t>::iterator itHarq = m_dlHarqProcessesDciBuffer.find (rnti);
          if (itHarq == m_dlHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << rnti);
            }

          DlDciListElement_s dci = (*itHarq).second.at (harqId);
          int rv = 0;
          if (dci.m_rv.size () == 1)
            {
              rv = dci.m_rv.at (0);
            }
          else
            {
              rv = (dci.m_rv.at (0) > dci.m_rv.at (1) ? dci.m_rv.at (0) : dci.m_rv.at (1));
            }

          if (rv == 3)
            {
              // maximum number of retx reached -> drop process
              NS_LOG_INFO ("Maximum number of retransmissions reached -> drop process");
              std::map <uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find (rnti);
              if (it == m_dlHarqProcessesStatus.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << m_dlInfoListBuffered.at (i).m_rnti);
                }
              (*it).second.at (harqId) = 0;
              std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (rnti);
              if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
                {
                  NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at (i).m_rnti);
                }
              for (uint16_t k = 0; k < (*itRlcPdu).second.size (); k++)
                {
                  (*itRlcPdu).second.at (k).at (harqId).clear ();
                }
              continue;
            }
          // check the feasibility of retransmitting on the same RBGs
          // translate the DCI to Spectrum framework
          std::vector <int> dciRbg;
          uint32_t mask = 0x1;
          NS_LOG_INFO ("Original RBGs " << dci.m_rbBitmap << " rnti " << dci.m_rnti);
          for (int j = 0; j < 32; j++)
            {
              if (((dci.m_rbBitmap & mask) >> j) == 1)
                {
                  dciRbg.push_back (j);
                  NS_LOG_INFO ("\t" << j);
                }
              mask = (mask << 1);
            }
          bool free = true;
          for (uint8_t j = 0; j < dciRbg.size (); j++)
            {
              if (rbgMap.at (dciRbg.at (j)) == true)
                {
                  free = false;
                  break;
                }
            }
          if (free)
            {
              // use the same RBGs for the retx
              // reserve RBGs
              for (uint8_t j = 0; j < dciRbg.size (); j++)
                {
                  rbgMap.at (dciRbg.at (j)) = true;
                  NS_LOG_INFO ("RBG " << dciRbg.at (j) << " assigned");
                  rbgAllocatedNum++;
                }

              NS_LOG_INFO (this << " Send retx in the same RBGs");
            }
          else
            {
              // find RBGs for sending HARQ retx
              uint8_t j = 0;
              uint8_t rbgId = (dciRbg.at (dciRbg.size () - 1) + 1) % rbgNum;
              uint8_t startRbg = dciRbg.at (dciRbg.size () - 1);
              std::vector <bool> rbgMapCopy = rbgMap;
              while ((j < dciRbg.size ())&&(startRbg != rbgId))
                {
                  if (rbgMapCopy.at (rbgId) == false)
                    {
                      rbgMapCopy.at (rbgId) = true;
                      dciRbg.at (j) = rbgId;
                      j++;
                    }
                  rbgId = (rbgId + 1) % rbgNum;
                }
              if (j == dciRbg.size ())
                {
                  // find new RBGs -> update DCI map
                  uint32_t rbgMask = 0;
                  for (uint16_t k = 0; k < dciRbg.size (); k++)
                    {
                      rbgMask = rbgMask + (0x1 << dciRbg.at (k));
                      rbgAllocatedNum++;
                    }
                  dci.m_rbBitmap = rbgMask;
                  rbgMap = rbgMapCopy;
                  NS_LOG_INFO (this << " Move retx in RBGs " << dciRbg.size ());
                }
              else
                {
                  // HARQ retx cannot be performed on this TTI -> store it
                  dlInfoListUntxed.push_back (m_dlInfoListBuffered.at (i));
                  NS_LOG_INFO (this << " No resource for this retx -> buffer it");
                }
            }
          // retrieve RLC PDU list for retx TBsize and update DCI
          BuildDataListElement_s newEl;
          std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (rnti);
          if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << rnti);
            }
          for (uint8_t j = 0; j < nLayers; j++)
            {
              if (retx.at (j))
                {
                  if (j >= dci.m_ndi.size ())
                    {
                      // for avoiding errors in MIMO transient phases
                      dci.m_ndi.push_back (0);
                      dci.m_rv.push_back (0);
                      dci.m_mcs.push_back (0);
                      dci.m_tbsSize.push_back (0);
                      NS_LOG_INFO (this << " layer " << (uint16_t)j << " no txed (MIMO transition)");
                    }
                  else
                    {
                      dci.m_ndi.at (j) = 0;
                      dci.m_rv.at (j)++;
                      (*itHarq).second.at (harqId).m_rv.at (j)++;
                      NS_LOG_INFO (this << " layer " << (uint16_t)j << " RV " << (uint16_t)dci.m_rv.at (j));
                    }
                }
              else
                {
                  // empty TB of layer j
                  dci.m_ndi.at (j) = 0;
                  dci.m_rv.at (j) = 0;
                  dci.m_mcs.at (j) = 0;
                  dci.m_tbsSize.at (j) = 0;
                  NS_LOG_INFO (this << " layer " << (uint16_t)j << " no retx");
                }
            }
          for (uint16_t k = 0; k < (*itRlcPdu).second.at (0).at (dci.m_harqProcess).size (); k++)
            {
              std::vector <struct RlcPduListElement_s> rlcPduListPerLc;
              for (uint8_t j = 0; j < nLayers; j++)
                {
                  if (retx.at (j))
                    {
                      if (j < dci.m_ndi.size ())
                        {
                          NS_LOG_INFO (" layer " << (uint16_t)j << " tb size " << dci.m_tbsSize.at (j));
                          rlcPduListPerLc.push_back ((*itRlcPdu).second.at (j).at (dci.m_harqProcess).at (k));
                        }
                    }
                  else
                    { // if no retx needed on layer j, push an RlcPduListElement_s object with m_size=0 to keep the size of rlcPduListPerLc vector = 2 in case of MIMO
                      NS_LOG_INFO (" layer " << (uint16_t)j << " tb size "<<dci.m_tbsSize.at (j));
                      RlcPduListElement_s emptyElement;
                      emptyElement.m_logicalChannelIdentity = (*itRlcPdu).second.at (j).at (dci.m_harqProcess).at (k).m_logicalChannelIdentity;
                      emptyElement.m_size = 0;
                      rlcPduListPerLc.push_back (emptyElement);
                    }
                }

              if (rlcPduListPerLc.size () > 0)
                {
                  newEl.m_rlcPduList.push_back (rlcPduListPerLc);
                }
            }
          newEl.m_rnti = rnti;
          newEl.m_dci = dci;
          (*itHarq).second.at (harqId).m_rv = dci.m_rv;
          // refresh timer
          std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer = m_dlHarqProcessesTimer.find (rnti);
          if (itHarqTimer== m_dlHarqProcessesTimer.end ())
            {
              NS_FATAL_ERROR ("Unable to find HARQ timer for RNTI " << (uint16_t)rnti);
            }
          (*itHarqTimer).second.at (harqId) = 0;
          ret.m_buildDataList.push_back (newEl);
          rntiAllocated.insert (rnti);
        }
      else
        {
          // update HARQ process status
          NS_LOG_INFO (this << " HARQ received ACK for UE " << m_dlInfoListBuffered.at (i).m_rnti);
          std::map <uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find (m_dlInfoListBuffered.at (i).m_rnti);
          if (it == m_dlHarqProcessesStatus.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << m_dlInfoListBuffered.at (i).m_rnti);
            }
          (*it).second.at (m_dlInfoListBuffered.at (i).m_harqProcessId) = 0;
          std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (m_dlInfoListBuffered.at (i).m_rnti);
          if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at (i).m_rnti);
            }
          for (uint16_t k = 0; k < (*itRlcPdu).second.size (); k++)
            {
              (*itRlcPdu).second.at (k).at (m_dlInfoListBuffered.at (i).m_harqProcessId).clear ();
            }
        }
    }
  m_dlInfoListBuffered.clear ();
  m_dlInfoListBuffered = dlInfoListUntxed;

  if (rbgAllocatedNum == rbgNum)
    {
      // all the RBGs are already allocated -> exit
      if ((ret.m_buildDataList.size () > 0) || (ret.m_buildRarList.size () > 0))
        {
          m_schedSapUser->SchedDlConfigInd (ret);
        }
      return;
    }

  std::map <uint16_t, RrsFlowPerf_t>::iterator itFlow;
  std::map <uint16_t, double> estAveThr;                                // store expected average throughput for UE
  std::map <uint16_t, double>::iterator itMax = estAveThr.begin ();
  std::map <uint16_t, double>::iterator it;
  std::map <uint16_t, int> rbgPerRntiLog;                               // record the number of RBG assigned to UE
  double metricMax = 0.0;
  for (itFlow = m_flowStatsDl.begin (); itFlow != m_flowStatsDl.end (); itFlow++)
    {
      // std::cout << "=-=-=-=-=-=-" << std::endl;
      // for(std::map <uint16_t, RrsFlowPerf_t>::iterator kk = m_flowStatsDl.begin (); kk != m_flowStatsDl.end (); kk++)
      // {
      //   std::cout << kk->first << std::endl;
      // }
      // std::cout << "=-=-=-=-=-=-" << std::endl;
      // std::cout << itFlow->first << std::endl;
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*itFlow).first);
      if ((itRnti != rntiAllocated.end ())||(!HarqProcessAvailability ((*itFlow).first)))
        {
          // UE already allocated for HARQ or without HARQ process available -> drop it
          if (itRnti != rntiAllocated.end ())
            {
              NS_LOG_DEBUG (this << " RNTI discared for HARQ tx" << (uint16_t)(*itFlow).first);
            }
          if (!HarqProcessAvailability ((*itFlow).first))
            {
              NS_LOG_DEBUG (this << " RNTI discared for HARQ id" << (uint16_t)(*itFlow).first);
            }
          continue;
        }

      // check first what are channel conditions for this UE, if CQI!=0
      std::map <uint16_t,uint8_t>::iterator itCqi;
      itCqi = m_p10CqiRxed.find ((*itFlow).first);
      std::map <uint16_t,uint8_t>::iterator itTxMode;
      itTxMode = m_uesTxMode.find ((*itFlow).first);
      
      if (itTxMode == m_uesTxMode.end ())
        {
          NS_FATAL_ERROR ("No Transmission Mode info on user " << (*itFlow).first);
        }
      int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);

      uint8_t cqiSum = 0;
      for (uint8_t j = 0; j < nLayer; j++)
        {
          if (itCqi == m_p10CqiRxed.end ())
            {
              cqiSum += 1;  // no info on this user -> lowest MCS
            }
          else
            {
              cqiSum = (*itCqi).second;
            }
        }
      if (cqiSum != 0)
        {
          estAveThr.insert (std::pair <uint16_t, double> ((*itFlow).first, (*itFlow).second.lastAveragedThroughput));
        }
      else
        {
          NS_LOG_INFO ("Skip this flow, CQI==0, rnti:"<<(*itFlow).first);
        }
    }
 
  if (estAveThr.size () != 0)
    {
      // Find UE with largest priority metric
      itMax = estAveThr.begin ();
      for (it = estAveThr.begin (); it != estAveThr.end (); it++)
        {
          // std::cout << it->first << std::endl;
          double metric =  1 / (*it).second;
          if (metric > metricMax)
            {
              metricMax = metric;
              itMax = it;
            }
          rbgPerRntiLog.insert (std::pair<uint16_t, int> ((*it).first, 1));
        }

      // The scheduler tries the best to achieve the equal throughput among all UEs
      int i = 0;
      do 
        {
          NS_LOG_INFO (this << " ALLOCATION for RBG " << i << " of " << rbgNum);
          if (rbgMap.at (i) == false)
            {
              // allocate one RBG to current UE
              std::map <uint16_t, std::vector <uint16_t> >::iterator itMap;
              std::vector <uint16_t> tempMap;
              itMap = allocationMap.find ((*itMax).first);
              if (itMap == allocationMap.end ())
                {
                  tempMap.push_back (i);
                  allocationMap.insert (std::pair <uint16_t, std::vector <uint16_t> > ((*itMax).first, tempMap));
                }
              else
                {
                  (*itMap).second.push_back (i);
                }

              // calculate expected throughput for current UE
              std::map <uint16_t,uint8_t>::iterator itCqi;
              itCqi = m_p10CqiRxed.find ((*itMax).first);
              std::map <uint16_t,uint8_t>::iterator itTxMode;
              itTxMode = m_uesTxMode.find ((*itMax).first);

              
              
              
              
              if (itTxMode == m_uesTxMode.end ())
                {
                  NS_FATAL_ERROR ("No Transmission Mode info on user " << (*itMax).first);
                }
              int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);
              std::vector <uint8_t> mcs;
              for (uint8_t j = 0; j < nLayer; j++) 
                {
                  if (itCqi == m_p10CqiRxed.end ())
                    {
                      mcs.push_back (0); // no info on this user -> lowest MCS
                    }
                  else
                    {
                      mcs.push_back (m_amc->GetMcsFromCqi ((*itCqi).second));
                    }
                }

              std::map <uint16_t,int>::iterator itRbgPerRntiLog;
              itRbgPerRntiLog = rbgPerRntiLog.find ((*itMax).first);
              std::map <uint16_t, RrsFlowPerf_t>::iterator itPastAveThr;
              itPastAveThr = m_flowStatsDl.find ((*itMax).first);
              uint32_t bytesTxed = 0;
              for (uint8_t j = 0; j < nLayer; j++)
                {
                  int tbSize = (m_amc->GetDlTbSizeFromMcs (mcs.at (0), (*itRbgPerRntiLog).second * rbgSize) / 8); // (size of TB in bytes according to table 7.1.7.2.1-1 of 36.213)
                  bytesTxed += tbSize;
                }
              double expectedAveThr = ((1.0 - (1.0 / m_timeWindow)) * (*itPastAveThr).second.lastAveragedThroughput) + ((1.0 / m_timeWindow) * (double)(bytesTxed / 0.001));

              int rbgPerRnti = (*itRbgPerRntiLog).second;
              rbgPerRnti++;
              rbgPerRntiLog[(*itMax).first] = rbgPerRnti;
              estAveThr[(*itMax).first] = expectedAveThr;

              // find new UE with largest priority metric
              metricMax = 0.0;
              for (it = estAveThr.begin (); it != estAveThr.end (); it++)
                {
                  double metric = 1 / (*it).second;
                  if (metric > metricMax)
                    {
                      itMax = it;
                      metricMax = metric;
                    }
                } // end for estAveThr

              rbgMap.at (i) = true;

            } // end for free RBGs

          i++;

        } 
      while ( i < rbgNum ); // end for RBGs

    } // end if estAveThr

  // reset TTI stats of users
  std::map <uint16_t, RrsFlowPerf_t>::iterator itStats;
  for (itStats = m_flowStatsDl.begin (); itStats != m_flowStatsDl.end (); itStats++)
    {
      (*itStats).second.lastTtiBytesTransmitted = 0;
    }

  // generate the transmission opportunities by grouping the RBGs of the same RNTI and
  // creating the correspondent DCIs
  std::map <uint16_t, std::vector <uint16_t> >::iterator itMap = allocationMap.begin ();
  while (itMap != allocationMap.end ())
    {
      // create new BuildDataListElement_s for this LC
      BuildDataListElement_s newEl;
      newEl.m_rnti = (*itMap).first;
      // create the DlDciListElement_s
      DlDciListElement_s newDci;
      newDci.m_rnti = (*itMap).first;
      newDci.m_harqProcess = UpdateHarqProcessId ((*itMap).first);

      uint16_t lcActives = LcActivePerFlow ((*itMap).first);
      NS_LOG_INFO (this << "Allocate user " << newEl.m_rnti << " rbg " << lcActives);
      if (lcActives == 0)
        {
          // Set to max value, to avoid divide by 0 below
          lcActives = (uint16_t)65535; // UINT16_MAX;
        }
      uint16_t RgbPerRnti = (*itMap).second.size ();
      std::map <uint16_t,uint8_t>::iterator itCqi;
      itCqi = m_p10CqiRxed.find ((*itMap).first);
      std::map <uint16_t,uint8_t>::iterator itTxMode;
      itTxMode = m_uesTxMode.find ((*itMap).first);
      if (itTxMode == m_uesTxMode.end ())
        {
          NS_FATAL_ERROR ("No Transmission Mode info on user " << (*itMap).first);
        }
      int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);

      uint32_t bytesTxed = 0;
      for (uint8_t j = 0; j < nLayer; j++)
        {
          if (itCqi == m_p10CqiRxed.end ())
            {
              newDci.m_mcs.push_back (0); // no info on this user -> lowest MCS
            }
          else
            {
              newDci.m_mcs.push_back ( m_amc->GetMcsFromCqi ((*itCqi).second) );
            }

          int tbSize = (m_amc->GetDlTbSizeFromMcs (newDci.m_mcs.at (j), RgbPerRnti * rbgSize) / 8); // (size of TB in bytes according to table 7.1.7.2.1-1 of 36.213)
          newDci.m_tbsSize.push_back (tbSize);
          bytesTxed += tbSize;
        }

      newDci.m_resAlloc = 0;  // only allocation type 0 at this stage
      newDci.m_rbBitmap = 0; // TBD (32 bit bitmap see 7.1.6 of 36.213)
      uint32_t rbgMask = 0;
      for (uint16_t k = 0; k < (*itMap).second.size (); k++)
        {
          rbgMask = rbgMask + (0x1 << (*itMap).second.at (k));
          NS_LOG_INFO (this << " Allocated RBG " << (*itMap).second.at (k));
        }
      newDci.m_rbBitmap = rbgMask; // (32 bit bitmap see 7.1.6 of 36.213)

      // create the rlc PDUs -> equally divide resources among actives LCs
      std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator itBufReq;
      for (itBufReq = m_rlcBufferReq_1.begin (); itBufReq != m_rlcBufferReq_1.end (); itBufReq++)
        {
          if (((*itBufReq).first.m_rnti == (*itMap).first)
              && (((*itBufReq).second.m_rlcTransmissionQueueSize > 0)
                  || ((*itBufReq).second.m_rlcRetransmissionQueueSize > 0)
                  || ((*itBufReq).second.m_rlcStatusPduSize > 0) ))
            {
              std::vector <struct RlcPduListElement_s> newRlcPduLe;
              for (uint8_t j = 0; j < nLayer; j++)
                {
                  RlcPduListElement_s newRlcEl;
                  newRlcEl.m_logicalChannelIdentity = (*itBufReq).first.m_lcId;
                  newRlcEl.m_size = newDci.m_tbsSize.at (j) / lcActives;
                  NS_LOG_INFO (this << " LCID " << (uint32_t) newRlcEl.m_logicalChannelIdentity << " size " << newRlcEl.m_size << " layer " << (uint16_t)j);
                  newRlcPduLe.push_back (newRlcEl);
                  UpdateDlRlcBufferInfo (newDci.m_rnti, newRlcEl.m_logicalChannelIdentity, newRlcEl.m_size);
                  if (m_harqOn == true)
                    {
                      // store RLC PDU list for HARQ
                      std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find ((*itMap).first);
                      if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
                        {
                          NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << (*itMap).first);
                        }
                      (*itRlcPdu).second.at (j).at (newDci.m_harqProcess).push_back (newRlcEl);
                    }
                }
              newEl.m_rlcPduList.push_back (newRlcPduLe);
            }
          if ((*itBufReq).first.m_rnti > (*itMap).first)
            {
              break;
            }
        }
      for (uint8_t j = 0; j < nLayer; j++)
        {
          newDci.m_ndi.push_back (1);
          newDci.m_rv.push_back (0);
        }

      newDci.m_tpc = 1; //1 is mapped to 0 in Accumulated Mode and to -1 in Absolute Mode

      newEl.m_dci = newDci;

      if (m_harqOn == true)
        {
          // store DCI for HARQ
          std::map <uint16_t, DlHarqProcessesDciBuffer_t>::iterator itDci = m_dlHarqProcessesDciBuffer.find (newEl.m_rnti);
          if (itDci == m_dlHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in DCI HARQ buffer for RNTI " << newEl.m_rnti);
            }
          (*itDci).second.at (newDci.m_harqProcess) = newDci;
          // refresh timer
          std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer =  m_dlHarqProcessesTimer.find (newEl.m_rnti);
          if (itHarqTimer== m_dlHarqProcessesTimer.end ())
            {
              NS_FATAL_ERROR ("Unable to find HARQ timer for RNTI " << (uint16_t)newEl.m_rnti);
            }
          (*itHarqTimer).second.at (newDci.m_harqProcess) = 0;
        }

      // ...more parameters -> ignored in this version

      ret.m_buildDataList.push_back (newEl);
      // update UE stats
      std::map <uint16_t, RrsFlowPerf_t>::iterator it;
      it = m_flowStatsDl.find ((*itMap).first);
      if (it != m_flowStatsDl.end ())
        {
          (*it).second.lastTtiBytesTransmitted = bytesTxed;
          NS_LOG_INFO (this << " UE total bytes txed " << (*it).second.lastTtiBytesTransmitted);


        }
      else
        {
          NS_FATAL_ERROR (this << " No Stats for this allocated UE");
        }

      itMap++;
    } // end while allocation
  ret.m_nrOfPdcchOfdmSymbols = 1;   /// \todo check correct value according the DCIs txed


  // update UEs stats
  NS_LOG_INFO (this << " Update UEs statistics");
  for (itStats = m_flowStatsDl.begin (); itStats != m_flowStatsDl.end (); itStats++)
    {
      (*itStats).second.totalBytesTransmitted += (*itStats).second.lastTtiBytesTransmitted;
      // update average throughput (see eq. 12.3 of Sec 12.3.1.2 of LTE – The UMTS Long Term Evolution, Ed Wiley)
      (*itStats).second.lastAveragedThroughput = ((1.0 - (1.0 / m_timeWindow)) * (*itStats).second.lastAveragedThroughput) + ((1.0 / m_timeWindow) * (double)((*itStats).second.lastTtiBytesTransmitted / 0.001));
      NS_LOG_INFO (this << " UE total bytes " << (*itStats).second.totalBytesTransmitted);
      NS_LOG_INFO (this << " UE average throughput " << (*itStats).second.lastAveragedThroughput);
      (*itStats).second.lastTtiBytesTransmitted = 0;
    }

  m_schedSapUser->SchedDlConfigInd (ret);


  return;
  }
  else if(m_flag == 3)
  {
    // std::cout << "执行3" << std::endl;

  NS_LOG_FUNCTION (this << " Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
  // API generated by RLC for triggering the scheduling of a DL subframe


  // evaluate the relative channel quality indicator for each UE per each RBG
  // (since we are using allocation type 0 the small unit of allocation is RBG)
  // Resource allocation type 0 (see sec 7.1.6.1 of 36.213)

  RefreshDlCqiMaps ();

  int rbgSize = GetRbgSize (m_cschedCellConfig.m_dlBandwidth);
  int rbgNum = m_cschedCellConfig.m_dlBandwidth / rbgSize;
  std::map <uint16_t, std::vector <uint16_t> > allocationMap; // RBs map per RNTI
  std::vector <bool> rbgMap;  // global RBGs map
  uint16_t rbgAllocatedNum = 0;
  std::set <uint16_t> rntiAllocated;
  rbgMap.resize (m_cschedCellConfig.m_dlBandwidth / rbgSize, false);

  rbgMap = m_ffrSapProvider->GetAvailableDlRbg ();
  for (std::vector<bool>::iterator it = rbgMap.begin (); it != rbgMap.end (); it++)
    {
      if ((*it) == true )
        {
          rbgAllocatedNum++;
        }
    }

  FfMacSchedSapUser::SchedDlConfigIndParameters ret;

  //   update UL HARQ proc id
  std::map <uint16_t, uint8_t>::iterator itProcId;
  for (itProcId = m_ulHarqCurrentProcessId.begin (); itProcId != m_ulHarqCurrentProcessId.end (); itProcId++)
    {
      (*itProcId).second = ((*itProcId).second + 1) % HARQ_PROC_NUM;
    }

  // RACH Allocation
  uint16_t rbAllocatedNum = 0;
  std::vector <bool> ulRbMap;
  ulRbMap.resize (m_cschedCellConfig.m_ulBandwidth, false);
  ulRbMap = m_ffrSapProvider->GetAvailableUlRbg ();
  uint8_t maxContinuousUlBandwidth = 0;
  uint8_t tmpMinBandwidth = 0;
  uint16_t ffrRbStartOffset = 0;
  uint16_t tmpFfrRbStartOffset = 0;
  uint16_t index = 0;

  for (std::vector<bool>::iterator it = ulRbMap.begin (); it != ulRbMap.end (); it++)
    {
      if ((*it) == true )
        {
          rbAllocatedNum++;
          if (tmpMinBandwidth > maxContinuousUlBandwidth)
            {
              maxContinuousUlBandwidth = tmpMinBandwidth;
              ffrRbStartOffset = tmpFfrRbStartOffset;
            }
          tmpMinBandwidth = 0;
        }
      else
        {
          if (tmpMinBandwidth == 0)
            {
              tmpFfrRbStartOffset = index;
            }
          tmpMinBandwidth++;
        }
      index++;
    }

  if (tmpMinBandwidth > maxContinuousUlBandwidth)
    {
      maxContinuousUlBandwidth = tmpMinBandwidth;
      ffrRbStartOffset = tmpFfrRbStartOffset;
    }

  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  uint16_t rbStart = 0;
  rbStart = ffrRbStartOffset;
  std::vector <struct RachListElement_s>::iterator itRach;
  for (itRach = m_rachList.begin (); itRach != m_rachList.end (); itRach++)
    {
      NS_ASSERT_MSG (m_amc->GetUlTbSizeFromMcs (m_ulGrantMcs, m_cschedCellConfig.m_ulBandwidth) > (*itRach).m_estimatedSize, " Default UL Grant MCS does not allow to send RACH messages");
      BuildRarListElement_s newRar;
      newRar.m_rnti = (*itRach).m_rnti;
      // DL-RACH Allocation
      // Ideal: no needs of configuring m_dci
      // UL-RACH Allocation
      newRar.m_grant.m_rnti = newRar.m_rnti;
      newRar.m_grant.m_mcs = m_ulGrantMcs;
      uint16_t rbLen = 1;
      uint16_t tbSizeBits = 0;
      // find lowest TB size that fits UL grant estimated size
      while ((tbSizeBits < (*itRach).m_estimatedSize) && (rbStart + rbLen < (ffrRbStartOffset + maxContinuousUlBandwidth)))
        {
          rbLen++;
          tbSizeBits = m_amc->GetUlTbSizeFromMcs (m_ulGrantMcs, rbLen);
        }
      if (tbSizeBits < (*itRach).m_estimatedSize)
        {
          // no more allocation space: finish allocation
          break;
        }
      newRar.m_grant.m_rbStart = rbStart;
      newRar.m_grant.m_rbLen = rbLen;
      newRar.m_grant.m_tbSize = tbSizeBits / 8;
      newRar.m_grant.m_hopping = false;
      newRar.m_grant.m_tpc = 0;
      newRar.m_grant.m_cqiRequest = false;
      newRar.m_grant.m_ulDelay = false;
      NS_LOG_INFO (this << " UL grant allocated to RNTI " << (*itRach).m_rnti << " rbStart " << rbStart << " rbLen " << rbLen << " MCS " << m_ulGrantMcs << " tbSize " << newRar.m_grant.m_tbSize);
      for (uint16_t i = rbStart; i < rbStart + rbLen; i++)
        {
          m_rachAllocationMap.at (i) = (*itRach).m_rnti;
        }

      if (m_harqOn == true)
        {
          // generate UL-DCI for HARQ retransmissions
          UlDciListElement_s uldci;
          uldci.m_rnti = newRar.m_rnti;
          uldci.m_rbLen = rbLen;
          uldci.m_rbStart = rbStart;
          uldci.m_mcs = m_ulGrantMcs;
          uldci.m_tbSize = tbSizeBits / 8;
          uldci.m_ndi = 1;
          uldci.m_cceIndex = 0;
          uldci.m_aggrLevel = 1;
          uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
          uldci.m_hopping = false;
          uldci.m_n2Dmrs = 0;
          uldci.m_tpc = 0; // no power control
          uldci.m_cqiRequest = false; // only period CQI at this stage
          uldci.m_ulIndex = 0; // TDD parameter
          uldci.m_dai = 1; // TDD parameter
          uldci.m_freqHopping = 0;
          uldci.m_pdcchPowerOffset = 0; // not used

          uint8_t harqId = 0;
          std::map <uint16_t, uint8_t>::iterator itProcId;
          itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
          if (itProcId == m_ulHarqCurrentProcessId.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
            }
          harqId = (*itProcId).second;
          std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
          if (itDci == m_ulHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
            }
          (*itDci).second.at (harqId) = uldci;
        }

      rbStart = rbStart + rbLen;
      ret.m_buildRarList.push_back (newRar);
    }
  m_rachList.clear ();


  // Process DL HARQ feedback
  RefreshHarqProcesses ();
  // retrieve past HARQ retx buffered
  if (m_dlInfoListBuffered.size () > 0)
    {
      if (params.m_dlInfoList.size () > 0)
        {
          NS_LOG_INFO (this << " Received DL-HARQ feedback");
          m_dlInfoListBuffered.insert (m_dlInfoListBuffered.end (), params.m_dlInfoList.begin (), params.m_dlInfoList.end ());
        }
    }
  else
    {
      if (params.m_dlInfoList.size () > 0)
        {
          m_dlInfoListBuffered = params.m_dlInfoList;
        }
    }
  if (m_harqOn == false)
    {
      // Ignore HARQ feedback
      m_dlInfoListBuffered.clear ();
    }
  std::vector <struct DlInfoListElement_s> dlInfoListUntxed;
  for (uint16_t i = 0; i < m_dlInfoListBuffered.size (); i++)
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find (m_dlInfoListBuffered.at (i).m_rnti);
      if (itRnti != rntiAllocated.end ())
        {
          // RNTI already allocated for retx
          continue;
        }
      uint8_t nLayers = m_dlInfoListBuffered.at (i).m_harqStatus.size ();
      std::vector <bool> retx;
      NS_LOG_INFO (this << " Processing DLHARQ feedback");
      if (nLayers == 1)
        {
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (0) == DlInfoListElement_s::NACK);
          retx.push_back (false);
        }
      else
        {
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (0) == DlInfoListElement_s::NACK);
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (1) == DlInfoListElement_s::NACK);
        }
      if (retx.at (0) || retx.at (1))
        {
          // retrieve HARQ process information
          uint16_t rnti = m_dlInfoListBuffered.at (i).m_rnti;
          uint8_t harqId = m_dlInfoListBuffered.at (i).m_harqProcessId;
          NS_LOG_INFO (this << " HARQ retx RNTI " << rnti << " harqId " << (uint16_t)harqId);
          std::map <uint16_t, DlHarqProcessesDciBuffer_t>::iterator itHarq = m_dlHarqProcessesDciBuffer.find (rnti);
          if (itHarq == m_dlHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << rnti);
            }

          DlDciListElement_s dci = (*itHarq).second.at (harqId);
          int rv = 0;
          if (dci.m_rv.size () == 1)
            {
              rv = dci.m_rv.at (0);
            }
          else
            {
              rv = (dci.m_rv.at (0) > dci.m_rv.at (1) ? dci.m_rv.at (0) : dci.m_rv.at (1));
            }

          if (rv == 3)
            {
              // maximum number of retx reached -> drop process
              NS_LOG_INFO ("Maximum number of retransmissions reached -> drop process");
              std::map <uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find (rnti);
              if (it == m_dlHarqProcessesStatus.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << m_dlInfoListBuffered.at (i).m_rnti);
                }
              (*it).second.at (harqId) = 0;
              std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (rnti);
              if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
                {
                  NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at (i).m_rnti);
                }
              for (uint16_t k = 0; k < (*itRlcPdu).second.size (); k++)
                {
                  (*itRlcPdu).second.at (k).at (harqId).clear ();
                }
              continue;
            }
          // check the feasibility of retransmitting on the same RBGs
          // translate the DCI to Spectrum framework
          std::vector <int> dciRbg;
          uint32_t mask = 0x1;
          NS_LOG_INFO ("Original RBGs " << dci.m_rbBitmap << " rnti " << dci.m_rnti);
          for (int j = 0; j < 32; j++)
            {
              if (((dci.m_rbBitmap & mask) >> j) == 1)
                {
                  dciRbg.push_back (j);
                  NS_LOG_INFO ("\t" << j);
                }
              mask = (mask << 1);
            }
          bool free = true;
          for (uint8_t j = 0; j < dciRbg.size (); j++)
            {
              if (rbgMap.at (dciRbg.at (j)) == true)
                {
                  free = false;
                  break;
                }
            }
          if (free)
            {
              // use the same RBGs for the retx
              // reserve RBGs
              for (uint8_t j = 0; j < dciRbg.size (); j++)
                {
                  rbgMap.at (dciRbg.at (j)) = true;
                  NS_LOG_INFO ("RBG " << dciRbg.at (j) << " assigned");
                  rbgAllocatedNum++;
                }

              NS_LOG_INFO (this << " Send retx in the same RBGs");
            }
          else
            {
              // find RBGs for sending HARQ retx
              uint8_t j = 0;
              uint8_t rbgId = (dciRbg.at (dciRbg.size () - 1) + 1) % rbgNum;
              uint8_t startRbg = dciRbg.at (dciRbg.size () - 1);
              std::vector <bool> rbgMapCopy = rbgMap;
              while ((j < dciRbg.size ())&&(startRbg != rbgId))
                {
                  if (rbgMapCopy.at (rbgId) == false)
                    {
                      rbgMapCopy.at (rbgId) = true;
                      dciRbg.at (j) = rbgId;
                      j++;
                    }
                  rbgId = (rbgId + 1) % rbgNum;
                }
              if (j == dciRbg.size ())
                {
                  // find new RBGs -> update DCI map
                  uint32_t rbgMask = 0;
                  for (uint16_t k = 0; k < dciRbg.size (); k++)
                    {
                      rbgMask = rbgMask + (0x1 << dciRbg.at (k));
                      rbgAllocatedNum++;
                    }
                  dci.m_rbBitmap = rbgMask;
                  rbgMap = rbgMapCopy;
                  NS_LOG_INFO (this << " Move retx in RBGs " << dciRbg.size ());
                }
              else
                {
                  // HARQ retx cannot be performed on this TTI -> store it
                  dlInfoListUntxed.push_back (m_dlInfoListBuffered.at (i));
                  NS_LOG_INFO (this << " No resource for this retx -> buffer it");
                }
            }
          // retrieve RLC PDU list for retx TBsize and update DCI
          BuildDataListElement_s newEl;
          std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (rnti);
          if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << rnti);
            }
          for (uint8_t j = 0; j < nLayers; j++)
            {
              if (retx.at (j))
                {
                  if (j >= dci.m_ndi.size ())
                    {
                      // for avoiding errors in MIMO transient phases
                      dci.m_ndi.push_back (0);
                      dci.m_rv.push_back (0);
                      dci.m_mcs.push_back (0);
                      dci.m_tbsSize.push_back (0);
                      NS_LOG_INFO (this << " layer " << (uint16_t)j << " no txed (MIMO transition)");
                    }
                  else
                    {
                      dci.m_ndi.at (j) = 0;
                      dci.m_rv.at (j)++;
                      (*itHarq).second.at (harqId).m_rv.at (j)++;
                      NS_LOG_INFO (this << " layer " << (uint16_t)j << " RV " << (uint16_t)dci.m_rv.at (j));
                    }
                }
              else
                {
                  // empty TB of layer j
                  dci.m_ndi.at (j) = 0;
                  dci.m_rv.at (j) = 0;
                  dci.m_mcs.at (j) = 0;
                  dci.m_tbsSize.at (j) = 0;
                  NS_LOG_INFO (this << " layer " << (uint16_t)j << " no retx");
                }
            }
          for (uint16_t k = 0; k < (*itRlcPdu).second.at (0).at (dci.m_harqProcess).size (); k++)
            {
              std::vector <struct RlcPduListElement_s> rlcPduListPerLc;
              for (uint8_t j = 0; j < nLayers; j++)
                {
                  if (retx.at (j))
                    {
                      if (j < dci.m_ndi.size ())
                        {
                          NS_LOG_INFO (" layer " << (uint16_t)j << " tb size " << dci.m_tbsSize.at (j));
                          rlcPduListPerLc.push_back ((*itRlcPdu).second.at (j).at (dci.m_harqProcess).at (k));
                        }
                    }
                  else
                    { // if no retx needed on layer j, push an RlcPduListElement_s object with m_size=0 to keep the size of rlcPduListPerLc vector = 2 in case of MIMO
                      NS_LOG_INFO (" layer " << (uint16_t)j << " tb size "<<dci.m_tbsSize.at (j));
                      RlcPduListElement_s emptyElement;
                      emptyElement.m_logicalChannelIdentity = (*itRlcPdu).second.at (j).at (dci.m_harqProcess).at (k).m_logicalChannelIdentity;
                      emptyElement.m_size = 0;
                      rlcPduListPerLc.push_back (emptyElement);
                    }
                }

              if (rlcPduListPerLc.size () > 0)
                {
                  newEl.m_rlcPduList.push_back (rlcPduListPerLc);
                }
            }
          newEl.m_rnti = rnti;
          newEl.m_dci = dci;
          (*itHarq).second.at (harqId).m_rv = dci.m_rv;
          // refresh timer
          std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer = m_dlHarqProcessesTimer.find (rnti);
          if (itHarqTimer== m_dlHarqProcessesTimer.end ())
            {
              NS_FATAL_ERROR ("Unable to find HARQ timer for RNTI " << (uint16_t)rnti);
            }
          (*itHarqTimer).second.at (harqId) = 0;
          ret.m_buildDataList.push_back (newEl);
          rntiAllocated.insert (rnti);
        }
      else
        {
          // update HARQ process status
          NS_LOG_INFO (this << " HARQ received ACK for UE " << m_dlInfoListBuffered.at (i).m_rnti);
          std::map <uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find (m_dlInfoListBuffered.at (i).m_rnti);
          if (it == m_dlHarqProcessesStatus.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << m_dlInfoListBuffered.at (i).m_rnti);
            }
          (*it).second.at (m_dlInfoListBuffered.at (i).m_harqProcessId) = 0;
          std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (m_dlInfoListBuffered.at (i).m_rnti);
          if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at (i).m_rnti);
            }
          for (uint16_t k = 0; k < (*itRlcPdu).second.size (); k++)
            {
              (*itRlcPdu).second.at (k).at (m_dlInfoListBuffered.at (i).m_harqProcessId).clear ();
            }
        }
    }
  m_dlInfoListBuffered.clear ();
  m_dlInfoListBuffered = dlInfoListUntxed;

  if (rbgAllocatedNum == rbgNum)
    {
      // all the RBGs are already allocated -> exit
      if ((ret.m_buildDataList.size () > 0) || (ret.m_buildRarList.size () > 0))
        {
          m_schedSapUser->SchedDlConfigInd (ret);
        }
      return;
    }


  std::map <uint16_t, RrsFlowPerf_t>::iterator it;
  std::map <uint16_t, RrsFlowPerf_t> tdUeSet; // the result of TD scheduler

  // schedulability check
  std::map <uint16_t, RrsFlowPerf_t> ueSet;
  for (it = m_flowStatsDl.begin (); it != m_flowStatsDl.end (); it++)
    {
      if( LcActivePerFlow ((*it).first) > 0 )
        {
          ueSet.insert(std::pair <uint16_t, RrsFlowPerf_t> ((*it).first, (*it).second));
        }
    }

  if (ueSet.size() != 0)
    { // has data in RLC buffer

      // Time Domain scheduler
      std::vector <std::pair<double, uint16_t> > ueSet1;
      std::vector <std::pair<double,uint16_t> > ueSet2;
      for (it = ueSet.begin (); it != ueSet.end (); it++)
        {
          std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
          if ((itRnti != rntiAllocated.end ())||(!HarqProcessAvailability ((*it).first)))
            {
              // UE already allocated for HARQ or without HARQ process available -> drop it
              if (itRnti != rntiAllocated.end ())
              {
                NS_LOG_DEBUG (this << " RNTI discared for HARQ tx" << (uint16_t)(*it).first);
              }
              if (!HarqProcessAvailability ((*it).first))
              {
                NS_LOG_DEBUG (this << " RNTI discared for HARQ id" << (uint16_t)(*it).first);
              }
              continue;
            }
    
          double metric = 0.0;
          if ((*it).second.lastAveragedThroughput < (*it).second.targetThroughput )
            {
        	    // calculate TD BET metric
              metric = 1 / (*it).second.lastAveragedThroughput;

              // check first what are channel conditions for this UE, if CQI!=0
              std::map <uint16_t,uint8_t>::iterator itCqi;
              itCqi = m_p10CqiRxed.find ((*it).first);
              std::map <uint16_t,uint8_t>::iterator itTxMode;
              itTxMode = m_uesTxMode.find ((*it).first);
              if (itTxMode == m_uesTxMode.end ())
                {
                  NS_FATAL_ERROR ("No Transmission Mode info on user " << (*it).first);
                }
              int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);

              uint8_t cqiSum = 0;
              for (uint8_t j = 0; j < nLayer; j++)
                {
                  if (itCqi == m_p10CqiRxed.end ())
                    {
                      cqiSum += 1;  // no info on this user -> lowest MCS
                    }
                  else
                    {
                      cqiSum = (*itCqi).second;
                    }
                }
              if (cqiSum != 0)
                {
                  ueSet1.push_back(std::pair<double, uint16_t> (metric, (*it).first));
                }
            }
          else
            {
              // calculate TD PF metric
              std::map <uint16_t,uint8_t>::iterator itCqi;
              itCqi = m_p10CqiRxed.find ((*it).first);
              std::map <uint16_t,uint8_t>::iterator itTxMode;
              itTxMode = m_uesTxMode.find ((*it).first);
              if (itTxMode == m_uesTxMode.end())
                {
                  NS_FATAL_ERROR ("No Transmission Mode info on user " << (*it).first);
                }
              int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);
              uint8_t wbCqi = 0;
              if (itCqi == m_p10CqiRxed.end())
                {
                  wbCqi = 1; // start with lowest value
                }
              else
                {
                  wbCqi = (*itCqi).second;
                }
    
              if (wbCqi > 0)
                {
                  if (LcActivePerFlow ((*it).first) > 0)
                    {
                      // this UE has data to transmit
                      double achievableRate = 0.0;
                      for (uint8_t k = 0; k < nLayer; k++) 
                        {
                          uint8_t mcs = 0; 
                          mcs = m_amc->GetMcsFromCqi (wbCqi);
                          achievableRate += ((m_amc->GetDlTbSizeFromMcs (mcs, rbgSize) / 8) / 0.001); // = TB size / TTI
                        }
    
                      metric = achievableRate / (*it).second.lastAveragedThroughput;
                   }
                  ueSet2.push_back(std::pair<double, uint16_t> (metric, (*it).first));
                } // end of wbCqi
            }
        }// end of ueSet
    
    
      if (ueSet1.size () != 0 || ueSet2.size () != 0)
        {
          // sorting UE in ueSet1 and ueSet1 in descending order based on their metric value
          std::sort (ueSet1.rbegin (), ueSet1.rend ());
          std::sort (ueSet2.rbegin (), ueSet2.rend ());
 
          // select UE set for frequency domain scheduler
          uint32_t nMux;
          if ( m_nMux > 0)
            nMux = m_nMux;
          else
            {
              // select half number of UE
              if (ueSet1.size() + ueSet2.size() <=2 )
                nMux = 1;
              else
                nMux = (int)((ueSet1.size() + ueSet2.size()) / 2) ; // TD scheduler only transfers half selected UE per RTT to TD scheduler
            }
          for (it = m_flowStatsDl.begin (); it != m_flowStatsDl.end (); it--)
           {
             std::vector <std::pair<double, uint16_t> >::iterator itSet;
             for (itSet = ueSet1.begin (); itSet != ueSet1.end () && nMux != 0; itSet++)
               {  
                 std::map <uint16_t, RrsFlowPerf_t>::iterator itUe;
                 itUe = m_flowStatsDl.find((*itSet).second);
                 tdUeSet.insert(std::pair<uint16_t, RrsFlowPerf_t> ( (*itUe).first, (*itUe).second ) );
                 nMux--;
               }
           
             if (nMux == 0)
               break;
        
             for (itSet = ueSet2.begin (); itSet != ueSet2.end () && nMux != 0; itSet++)
               {  
                 std::map <uint16_t, RrsFlowPerf_t>::iterator itUe;
                 itUe = m_flowStatsDl.find((*itSet).second);
                 tdUeSet.insert(std::pair<uint16_t, RrsFlowPerf_t> ( (*itUe).first, (*itUe).second ) );
                 nMux--;
               }
        
             if (nMux == 0)
               break;
        
           } // end of m_flowStatsDl
        
        
          if ( m_fdSchedulerType.compare("CoItA") == 0)
            {
              // FD scheduler: Carrier over Interference to Average (CoItA)
              std::map < uint16_t, uint8_t > sbCqiSum;
              for (it = tdUeSet.begin (); it != tdUeSet.end (); it++)
                {
                  uint8_t sum = 0;
                  for (int i = 0; i < rbgNum; i++)
                    {
                      std::map <uint16_t,SbMeasResult_s>::iterator itCqi;
                      itCqi = m_a30CqiRxed.find ((*it).first);
                      std::map <uint16_t,uint8_t>::iterator itTxMode;
                      itTxMode = m_uesTxMode.find ((*it).first);
                      if (itTxMode == m_uesTxMode.end ())
                        {
                          NS_FATAL_ERROR ("No Transmission Mode info on user " << (*it).first);
                        }
                      int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);
                      std::vector <uint8_t> sbCqis;
                      if (itCqi == m_a30CqiRxed.end ())
                        {
                          for (uint8_t k = 0; k < nLayer; k++)
                            {
                              sbCqis.push_back (1);  // start with lowest value
                            }
                        }
                      else
                        {
                          sbCqis = (*itCqi).second.m_higherLayerSelected.at (i).m_sbCqi;
                        }
        
                      uint8_t cqi1 = sbCqis.at (0);
                      uint8_t cqi2 = 0;
                      if (sbCqis.size () > 1)
                        {
                          cqi2 = sbCqis.at (1);
                        }
            
                      uint8_t sbCqi = 0;
                      if ((cqi1 > 0)||(cqi2 > 0)) // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
                        {
                          for (uint8_t k = 0; k < nLayer; k++) 
                            {
                              if (sbCqis.size () > k)
                                {                       
           	                  sbCqi = sbCqis.at(k);
                                }
                              else
                                {
                                  // no info on this subband 
                                  sbCqi = 0;
                                }
                              sum += sbCqi;
                            }
                        }   // end if cqi
                    }// end of rbgNum
              
                  sbCqiSum.insert (std::pair<uint16_t, uint8_t> ((*it).first, sum));
                }// end tdUeSet
        
              for (int i = 0; i < rbgNum; i++)
                {
                  if (rbgMap.at (i) == true)
                    continue;

                  std::map <uint16_t, RrsFlowPerf_t>::iterator itMax = tdUeSet.end ();
                  double metricMax = 0.0;
                  for (it = tdUeSet.begin (); it != tdUeSet.end (); it++)
                    {
                      if ((m_ffrSapProvider->IsDlRbgAvailableForUe (i, (*it).first)) == false)
                        continue;

                      // calculate PF weight 
                      double weight = (*it).second.targetThroughput / (*it).second.lastAveragedThroughput;
                      if (weight < 1.0)
                        weight = 1.0;
        
                      std::map < uint16_t, uint8_t>::iterator itSbCqiSum;
                      itSbCqiSum = sbCqiSum.find((*it).first);
        
                      std::map <uint16_t,SbMeasResult_s>::iterator itCqi;
                      itCqi = m_a30CqiRxed.find ((*it).first);
                      std::map <uint16_t,uint8_t>::iterator itTxMode;
                      itTxMode = m_uesTxMode.find ((*it).first);
                      if (itTxMode == m_uesTxMode.end())
                        {
                          NS_FATAL_ERROR ("No Transmission Mode info on user " << (*it).first);
                        }
                      int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);
                      std::vector <uint8_t> sbCqis;
                      if (itCqi == m_a30CqiRxed.end ())
                        {
                          for (uint8_t k = 0; k < nLayer; k++)
                            {
                              sbCqis.push_back (1);  // start with lowest value
                            }
                        }
                      else
                        {
                          sbCqis = (*itCqi).second.m_higherLayerSelected.at (i).m_sbCqi;
                        }
        
                      uint8_t cqi1 = sbCqis.at( 0);
                      uint8_t cqi2 = 0;
                      if (sbCqis.size () > 1)
                        {
                          cqi2 = sbCqis.at(1);
                        }
            
                      uint8_t sbCqi = 0;
                      double colMetric = 0.0;
                      if ((cqi1 > 0)||(cqi2 > 0)) // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
                        {
                          for (uint8_t k = 0; k < nLayer; k++) 
                            {
                              if (sbCqis.size () > k)
                                {                       
                                  sbCqi = sbCqis.at(k);
                                }
                              else
                                {
                                  // no info on this subband 
                                  sbCqi = 0;
                                }
                              colMetric += (double)sbCqi / (double)(*itSbCqiSum).second;
           	                } 
                        }   // end if cqi
        
                      double metric = 0.0;
                      if (colMetric != 0)
                        metric= weight * colMetric;
                      else
                        metric = 1;
        
                      if (metric > metricMax )
                        {
                          metricMax = metric;
                          itMax = it;
                        }
                    } // end of tdUeSet

                  if (itMax == tdUeSet.end ())
                    {
                      // no UE available for downlink
                    }
                  else
                    {
                      allocationMap[(*itMax).first].push_back (i);
                      rbgMap.at (i) = true;
                    }
                }// end of rbgNum
        
            }// end of CoIta
        
        
          if ( m_fdSchedulerType.compare("PFsch") == 0)
            {
              // FD scheduler: Proportional Fair scheduled (PFsch)
              for (int i = 0; i < rbgNum; i++)
                {
                  if (rbgMap.at (i) == true)
                    continue;

                  std::map <uint16_t, RrsFlowPerf_t>::iterator itMax = tdUeSet.end ();
                  double metricMax = 0.0;
                  for (it = tdUeSet.begin (); it != tdUeSet.end (); it++)
                    {
                      if ((m_ffrSapProvider->IsDlRbgAvailableForUe (i, (*it).first)) == false)
                        continue;
                      // calculate PF weight 
                      double weight = (*it).second.targetThroughput / (*it).second.lastAveragedThroughput;
                      if (weight < 1.0)
                        weight = 1.0;
        
                      std::map <uint16_t,SbMeasResult_s>::iterator itCqi;
                      itCqi = m_a30CqiRxed.find ((*it).first);
                      std::map <uint16_t,uint8_t>::iterator itTxMode;
                      itTxMode = m_uesTxMode.find ((*it).first);
                      if (itTxMode == m_uesTxMode.end())
                        {
                          NS_FATAL_ERROR ("No Transmission Mode info on user " << (*it).first);
                        }
                      int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);
                      std::vector <uint8_t> sbCqis;
                      if (itCqi == m_a30CqiRxed.end ())
                        {
                          for (uint8_t k = 0; k < nLayer; k++)
                            {
                              sbCqis.push_back (1);  // start with lowest value
                            }
                        }
                      else
                        {
                          sbCqis = (*itCqi).second.m_higherLayerSelected.at (i).m_sbCqi;
                        }
        
                      uint8_t cqi1 = sbCqis.at(0);
                      uint8_t cqi2 = 0;
                      if (sbCqis.size () > 1)
                        {
                          cqi2 = sbCqis.at(1);
                        }
                
                      double schMetric = 0.0;
                      if ((cqi1 > 0)||(cqi2 > 0)) // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
                        {
                          double achievableRate = 0.0;
                          for (uint8_t k = 0; k < nLayer; k++) 
                            {
                              uint8_t mcs = 0;
                              if (sbCqis.size () > k)
                                {                       
                                  mcs = m_amc->GetMcsFromCqi (sbCqis.at (k));
                                }
                              else
                                {
                                  // no info on this subband  -> worst MCS
                                  mcs = 0;
                                }
                              achievableRate += ((m_amc->GetDlTbSizeFromMcs (mcs, rbgSize) / 8) / 0.001); // = TB size / TTI
            	  	    }
                          schMetric = achievableRate / (*it).second.secondLastAveragedThroughput;
                        }   // end if cqi
         
                      double metric = 0.0;
                      metric= weight * schMetric;
         
                      if (metric > metricMax )
                        {
                          metricMax = metric;
                          itMax = it;
                        }
                    } // end of tdUeSet

                  if (itMax == tdUeSet.end ())
                    {
                      // no UE available for downlink 
                    }
                  else
                    {
                      allocationMap[(*itMax).first].push_back (i);
                      rbgMap.at (i) = true;
                    }
         
                }// end of rbgNum
        
            } // end of PFsch

        } // end if ueSet1 || ueSet2
    
    } // end if ueSet



  // reset TTI stats of users
  std::map <uint16_t, RrsFlowPerf_t>::iterator itStats;
  for (itStats = m_flowStatsDl.begin (); itStats != m_flowStatsDl.end (); itStats++)
    {
      (*itStats).second.lastTtiBytesTransmitted = 0;
    }

  // generate the transmission opportunities by grouping the RBGs of the same RNTI and
  // creating the correspondent DCIs
  std::map <uint16_t, std::vector <uint16_t> >::iterator itMap = allocationMap.begin ();
  while (itMap != allocationMap.end ())
    {
      // create new BuildDataListElement_s for this LC
      BuildDataListElement_s newEl;
      newEl.m_rnti = (*itMap).first;
      // create the DlDciListElement_s
      DlDciListElement_s newDci;
      newDci.m_rnti = (*itMap).first;
      newDci.m_harqProcess = UpdateHarqProcessId ((*itMap).first);

      uint16_t lcActives = LcActivePerFlow ((*itMap).first);
      NS_LOG_INFO (this << "Allocate user " << newEl.m_rnti << " rbg " << lcActives);
      if (lcActives == 0)
        {
          // Set to max value, to avoid divide by 0 below
          lcActives = (uint16_t)65535; // UINT16_MAX;
        }
      uint16_t RgbPerRnti = (*itMap).second.size ();
      std::map <uint16_t,SbMeasResult_s>::iterator itCqi;
      itCqi = m_a30CqiRxed.find ((*itMap).first);
      std::map <uint16_t,uint8_t>::iterator itTxMode;
      itTxMode = m_uesTxMode.find ((*itMap).first);
      if (itTxMode == m_uesTxMode.end ())
        {
          NS_FATAL_ERROR ("No Transmission Mode info on user " << (*itMap).first);
        }
      int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);
      std::vector <uint8_t> worstCqi (2, 15);
      if (itCqi != m_a30CqiRxed.end ())
        {
          for (uint16_t k = 0; k < (*itMap).second.size (); k++)
            {
              if ((*itCqi).second.m_higherLayerSelected.size () > (*itMap).second.at (k))
                {
                  NS_LOG_INFO (this << " RBG " << (*itMap).second.at (k) << " CQI " << (uint16_t)((*itCqi).second.m_higherLayerSelected.at ((*itMap).second.at (k)).m_sbCqi.at (0)) );
                  for (uint8_t j = 0; j < nLayer; j++)
                    {
                      if ((*itCqi).second.m_higherLayerSelected.at ((*itMap).second.at (k)).m_sbCqi.size () > j)
                        {
                          if (((*itCqi).second.m_higherLayerSelected.at ((*itMap).second.at (k)).m_sbCqi.at (j)) < worstCqi.at (j))
                            {
                              worstCqi.at (j) = ((*itCqi).second.m_higherLayerSelected.at ((*itMap).second.at (k)).m_sbCqi.at (j));
                            }
                        }
                      else
                        {
                          // no CQI for this layer of this suband -> worst one
                          worstCqi.at (j) = 1;
                        }
                    }
                }
              else
                {
                  for (uint8_t j = 0; j < nLayer; j++)
                    {
                      worstCqi.at (j) = 1; // try with lowest MCS in RBG with no info on channel
                    }
                }
            }
        }
      else
        {
          for (uint8_t j = 0; j < nLayer; j++)
            {
              worstCqi.at (j) = 1; // try with lowest MCS in RBG with no info on channel
            }
        }
      for (uint8_t j = 0; j < nLayer; j++)
        {
          NS_LOG_INFO (this << " Layer " << (uint16_t)j << " CQI selected " << (uint16_t)worstCqi.at (j));
        }
      uint32_t bytesTxed = 0;
      for (uint8_t j = 0; j < nLayer; j++)
        {
          newDci.m_mcs.push_back (m_amc->GetMcsFromCqi (worstCqi.at (j)));
          int tbSize = (m_amc->GetDlTbSizeFromMcs (newDci.m_mcs.at (j), RgbPerRnti * rbgSize) / 8); // (size of TB in bytes according to table 7.1.7.2.1-1 of 36.213)
          newDci.m_tbsSize.push_back (tbSize);
          NS_LOG_INFO (this << " Layer " << (uint16_t)j << " MCS selected" << m_amc->GetMcsFromCqi (worstCqi.at (j)));
          bytesTxed += tbSize;
        }

      newDci.m_resAlloc = 0;  // only allocation type 0 at this stage
      newDci.m_rbBitmap = 0; // TBD (32 bit bitmap see 7.1.6 of 36.213)
      uint32_t rbgMask = 0;
      for (uint16_t k = 0; k < (*itMap).second.size (); k++)
        {
          rbgMask = rbgMask + (0x1 << (*itMap).second.at (k));
          NS_LOG_INFO (this << " Allocated RBG " << (*itMap).second.at (k));
        }
      newDci.m_rbBitmap = rbgMask; // (32 bit bitmap see 7.1.6 of 36.213)

      // create the rlc PDUs -> equally divide resources among actives LCs
      std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator itBufReq;
      for (itBufReq = m_rlcBufferReq_1.begin (); itBufReq != m_rlcBufferReq_1.end (); itBufReq++)
        {
          if (((*itBufReq).first.m_rnti == (*itMap).first)
              && (((*itBufReq).second.m_rlcTransmissionQueueSize > 0)
                  || ((*itBufReq).second.m_rlcRetransmissionQueueSize > 0)
                  || ((*itBufReq).second.m_rlcStatusPduSize > 0) ))
            {
              std::vector <struct RlcPduListElement_s> newRlcPduLe;
              for (uint8_t j = 0; j < nLayer; j++)
                {
                  RlcPduListElement_s newRlcEl;
                  newRlcEl.m_logicalChannelIdentity = (*itBufReq).first.m_lcId;
                  newRlcEl.m_size = newDci.m_tbsSize.at (j) / lcActives;
                  NS_LOG_INFO (this << " LCID " << (uint32_t) newRlcEl.m_logicalChannelIdentity << " size " << newRlcEl.m_size << " layer " << (uint16_t)j);
                  newRlcPduLe.push_back (newRlcEl);
                  UpdateDlRlcBufferInfo (newDci.m_rnti, newRlcEl.m_logicalChannelIdentity, newRlcEl.m_size);
                  if (m_harqOn == true)
                    {
                      // store RLC PDU list for HARQ
                      std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find ((*itMap).first);
                      if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
                        {
                          NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << (*itMap).first);
                        }
                      (*itRlcPdu).second.at (j).at (newDci.m_harqProcess).push_back (newRlcEl);
                    }
                }
              newEl.m_rlcPduList.push_back (newRlcPduLe);
            }
          if ((*itBufReq).first.m_rnti > (*itMap).first)
            {
              break;
            }
        }
      for (uint8_t j = 0; j < nLayer; j++)
        {
          newDci.m_ndi.push_back (1);
          newDci.m_rv.push_back (0);
        }

      newDci.m_tpc = m_ffrSapProvider->GetTpc ((*itMap).first);

      newEl.m_dci = newDci;

      if (m_harqOn == true)
        {
          // store DCI for HARQ
          std::map <uint16_t, DlHarqProcessesDciBuffer_t>::iterator itDci = m_dlHarqProcessesDciBuffer.find (newEl.m_rnti);
          if (itDci == m_dlHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in DCI HARQ buffer for RNTI " << newEl.m_rnti);
            }
          (*itDci).second.at (newDci.m_harqProcess) = newDci;
          // refresh timer
          std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer =  m_dlHarqProcessesTimer.find (newEl.m_rnti);
          if (itHarqTimer== m_dlHarqProcessesTimer.end ())
            {
              NS_FATAL_ERROR ("Unable to find HARQ timer for RNTI " << (uint16_t)newEl.m_rnti);
            }
          (*itHarqTimer).second.at (newDci.m_harqProcess) = 0;
        }

      // ...more parameters -> ignored in this version

      ret.m_buildDataList.push_back (newEl);
      // update UE stats
      std::map <uint16_t, RrsFlowPerf_t>::iterator it;
      it = m_flowStatsDl.find ((*itMap).first);
      if (it != m_flowStatsDl.end ())
        {
          (*it).second.lastTtiBytesTransmitted = bytesTxed;
          NS_LOG_INFO (this << " UE total bytes txed " << (*it).second.lastTtiBytesTransmitted);


        }
      else
        {
          NS_FATAL_ERROR (this << " No Stats for this allocated UE");
        }

      itMap++;
    } // end while allocation
  ret.m_nrOfPdcchOfdmSymbols = 1;   /// \todo check correct value according the DCIs txed


  // update UEs stats
  NS_LOG_INFO (this << " Update UEs statistics");
  for (itStats = m_flowStatsDl.begin (); itStats != m_flowStatsDl.end (); itStats++)
    { 
      std::map <uint16_t, RrsFlowPerf_t>::iterator itUeScheduleted = tdUeSet.end();
      itUeScheduleted = tdUeSet.find((*itStats).first);
      if (itUeScheduleted != tdUeSet.end())
        {
          (*itStats).second.secondLastAveragedThroughput = ((1.0 - (1 / m_timeWindow)) * (*itStats).second.secondLastAveragedThroughput) + ((1 / m_timeWindow) * (double)((*itStats).second.lastTtiBytesTransmitted / 0.001));
        }

      (*itStats).second.totalBytesTransmitted += (*itStats).second.lastTtiBytesTransmitted;
      // update average throughput (see eq. 12.3 of Sec 12.3.1.2 of LTE – The UMTS Long Term Evolution, Ed Wiley)
      (*itStats).second.lastAveragedThroughput = ((1.0 - (1.0 / m_timeWindow)) * (*itStats).second.lastAveragedThroughput) + ((1.0 / m_timeWindow) * (double)((*itStats).second.lastTtiBytesTransmitted / 0.001));
      (*itStats).second.lastTtiBytesTransmitted = 0;
    }


  m_schedSapUser->SchedDlConfigInd (ret);


  return;
  }




  else
  {
    // std::cout << "执行1" <<std::endl;
    NS_LOG_FUNCTION (this << " Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
  // API generated by RLC for triggering the scheduling of a DL subframe
  // evaluate the relative channel quality indicator for each UE per each RBG
  // (since we are using allocation type 0 the small unit of allocation is RBG)
  // Resource allocation type 0 (see sec 7.1.6.1 of 36.213)

  RefreshDlCqiMaps ();

  int rbgSize = GetRbgSize (m_cschedCellConfig.m_dlBandwidth);
  int numberOfRBGs = m_cschedCellConfig.m_dlBandwidth / rbgSize;
  std::map <uint16_t, std::multimap <uint8_t, qos_rb_and_CQI_assigned_to_lc> > allocationMapPerRntiPerLCId;
  std::map <uint16_t, std::multimap <uint8_t, qos_rb_and_CQI_assigned_to_lc> >::iterator itMap;
  allocationMapPerRntiPerLCId.clear ();
  bool(*key_function_pointer_groups)(int,int) = CqaGroupDescComparator_;
  t_map_HOLgroupToUEs map_GBRHOLgroupToUE (key_function_pointer_groups);
  t_map_HOLgroupToUEs map_nonGBRHOLgroupToUE (key_function_pointer_groups);
  int grouping_parameter = 1000;
  double tolerance = 1.1;
  std::map<LteFlowId_t,int> UEtoHOL;
  std::vector <bool> rbgMap;  // global RBGs map
  uint16_t rbgAllocatedNum = 0;
  std::set <uint16_t> rntiAllocated;
  rbgMap.resize (m_cschedCellConfig.m_dlBandwidth / rbgSize, false);

  rbgMap = m_ffrSapProvider->GetAvailableDlRbg ();
  for (std::vector<bool>::iterator it = rbgMap.begin (); it != rbgMap.end (); it++)
    {
      if ((*it) == true )
        {
          rbgAllocatedNum++;
        }
    }

  FfMacSchedSapUser::SchedDlConfigIndParameters ret;

  //   update UL HARQ proc id
  std::map <uint16_t, uint8_t>::iterator itProcId;
  for (itProcId = m_ulHarqCurrentProcessId.begin (); itProcId != m_ulHarqCurrentProcessId.end (); itProcId++)
    {
      (*itProcId).second = ((*itProcId).second + 1) % HARQ_PROC_NUM;
    }


  // RACH Allocation
  uint16_t rbAllocatedNum = 0;
  std::vector <bool> ulRbMap;
  ulRbMap.resize (m_cschedCellConfig.m_ulBandwidth, false);
  ulRbMap = m_ffrSapProvider->GetAvailableUlRbg ();
  uint8_t maxContinuousUlBandwidth = 0;
  uint8_t tmpMinBandwidth = 0;
  uint16_t ffrRbStartOffset = 0;
  uint16_t tmpFfrRbStartOffset = 0;
  uint16_t index = 0;

  for (std::vector<bool>::iterator it = ulRbMap.begin (); it != ulRbMap.end (); it++)
    {
      if ((*it) == true )
        {
          rbAllocatedNum++;
          if (tmpMinBandwidth > maxContinuousUlBandwidth)
            {
              maxContinuousUlBandwidth = tmpMinBandwidth;
              ffrRbStartOffset = tmpFfrRbStartOffset;
            }
          tmpMinBandwidth = 0;
        }
      else
        {
          if (tmpMinBandwidth == 0)
            {
              tmpFfrRbStartOffset = index;
            }
          tmpMinBandwidth++;
        }
      index++;
    }

  if (tmpMinBandwidth > maxContinuousUlBandwidth)
    {
      maxContinuousUlBandwidth = tmpMinBandwidth;
      ffrRbStartOffset = tmpFfrRbStartOffset;
    }

  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  uint16_t rbStart = 0;
  rbStart = ffrRbStartOffset;
  std::vector <struct RachListElement_s>::iterator itRach;

  for (itRach = m_rachList.begin (); itRach != m_rachList.end (); itRach++)
    {

      NS_ASSERT_MSG (m_amc->GetUlTbSizeFromMcs (m_ulGrantMcs, m_cschedCellConfig.m_ulBandwidth) > (*itRach).m_estimatedSize, " Default UL Grant MCS does not allow to send RACH messages");
      BuildRarListElement_s newRar;
      newRar.m_rnti = (*itRach).m_rnti;
      // DL-RACH Allocation
      // Ideal: no needs of configuring m_dci
      // UL-RACH Allocation
      newRar.m_grant.m_rnti = newRar.m_rnti;
      newRar.m_grant.m_mcs = m_ulGrantMcs;
      uint16_t rbLen = 1;
      uint16_t tbSizeBits = 0;
      // find lowest TB size that fits UL grant estimated size
      while ((tbSizeBits < (*itRach).m_estimatedSize) && (rbStart + rbLen < (ffrRbStartOffset + maxContinuousUlBandwidth)))
        {
          rbLen++;
          tbSizeBits = m_amc->GetUlTbSizeFromMcs (m_ulGrantMcs, rbLen);
        }
      if (tbSizeBits < (*itRach).m_estimatedSize)
        {
          // no more allocation space: finish allocation
          break;
        }
      newRar.m_grant.m_rbStart = rbStart;
      newRar.m_grant.m_rbLen = rbLen;
      newRar.m_grant.m_tbSize = tbSizeBits / 8;
      newRar.m_grant.m_hopping = false;
      newRar.m_grant.m_tpc = 0;
      newRar.m_grant.m_cqiRequest = false;
      newRar.m_grant.m_ulDelay = false;
      NS_LOG_INFO (this << " UL grant allocated to RNTI " << (*itRach).m_rnti << " rbStart " << rbStart << " rbLen " << rbLen << " MCS " << m_ulGrantMcs << " tbSize " << newRar.m_grant.m_tbSize);
      for (uint16_t i = rbStart; i < rbStart + rbLen; i++)
        {
          m_rachAllocationMap.at (i) = (*itRach).m_rnti;
        }
      
      if (m_harqOn == true)
        {
          // generate UL-DCI for HARQ retransmissions
          UlDciListElement_s uldci;
          uldci.m_rnti = newRar.m_rnti;
          uldci.m_rbLen = rbLen;
          uldci.m_rbStart = rbStart;
          uldci.m_mcs = m_ulGrantMcs;
          uldci.m_tbSize = tbSizeBits / 8;
          uldci.m_ndi = 1;
          uldci.m_cceIndex = 0;
          uldci.m_aggrLevel = 1;
          uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
          uldci.m_hopping = false;
          uldci.m_n2Dmrs = 0;
          uldci.m_tpc = 0; // no power control
          uldci.m_cqiRequest = false; // only period CQI at this stage
          uldci.m_ulIndex = 0; // TDD parameter
          uldci.m_dai = 1; // TDD parameter
          uldci.m_freqHopping = 0;
          uldci.m_pdcchPowerOffset = 0; // not used

          uint8_t harqId = 0;
          std::map <uint16_t, uint8_t>::iterator itProcId;
          itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
          if (itProcId == m_ulHarqCurrentProcessId.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
            }
          harqId = (*itProcId).second;
          std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
          if (itDci == m_ulHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
            }
          (*itDci).second.at (harqId) = uldci;
        }
      
      rbStart = rbStart + rbLen;
      ret.m_buildRarList.push_back (newRar);
    }
  m_rachList.clear ();






  // Process DL HARQ feedback
  RefreshHarqProcesses ();
  // retrieve past HARQ retx buffered
  if (m_dlInfoListBuffered.size () > 0)
    {
      if (params.m_dlInfoList.size () > 0)
        {
          NS_LOG_INFO (this << " Received DL-HARQ feedback");
          m_dlInfoListBuffered.insert (m_dlInfoListBuffered.end (), params.m_dlInfoList.begin (), params.m_dlInfoList.end ());
        }
    }
  else
    {
      if (params.m_dlInfoList.size () > 0)
        {
          m_dlInfoListBuffered = params.m_dlInfoList;
        }
    }
  if (m_harqOn == false)
    {
      // Ignore HARQ feedback
      m_dlInfoListBuffered.clear ();
    }



  std::vector <struct DlInfoListElement_s> dlInfoListUntxed;
  for (uint16_t i = 0; i < m_dlInfoListBuffered.size (); i++)
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find (m_dlInfoListBuffered.at (i).m_rnti);
      if (itRnti != rntiAllocated.end ())
        {
          // RNTI already allocated for retx
          continue;
        }
      uint8_t nLayers = m_dlInfoListBuffered.at (i).m_harqStatus.size ();
      std::vector <bool> retx;
      NS_LOG_INFO (this << " Processing DLHARQ feedback");
      if (nLayers == 1)
        {
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (0) == DlInfoListElement_s::NACK);
          retx.push_back (false);
        }
      else
        {
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (0) == DlInfoListElement_s::NACK);
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (1) == DlInfoListElement_s::NACK);
        }
      if (retx.at (0) || retx.at (1))
        {
          // retrieve HARQ process information
          uint16_t rnti = m_dlInfoListBuffered.at (i).m_rnti;
          uint8_t harqId = m_dlInfoListBuffered.at (i).m_harqProcessId;
          NS_LOG_INFO (this << " HARQ retx RNTI " << rnti << " harqId " << (uint16_t)harqId);
          std::map <uint16_t, DlHarqProcessesDciBuffer_t>::iterator itHarq = m_dlHarqProcessesDciBuffer.find (rnti);
          if (itHarq == m_dlHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << rnti);
            }

          DlDciListElement_s dci = (*itHarq).second.at (harqId);
          int rv = 0;
          if (dci.m_rv.size () == 1)
            {
              rv = dci.m_rv.at (0);
            }
          else
            {
              rv = (dci.m_rv.at (0) > dci.m_rv.at (1) ? dci.m_rv.at (0) : dci.m_rv.at (1));
            }

          if (rv == 3)
            {
              // maximum number of retx reached -> drop process
              NS_LOG_INFO ("Maximum number of retransmissions reached -> drop process");
              std::map <uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find (rnti);
              if (it == m_dlHarqProcessesStatus.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << m_dlInfoListBuffered.at (i).m_rnti);
                }
              (*it).second.at (harqId) = 0;
              std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (rnti);
              if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
                {
                  NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at (i).m_rnti);
                }
              for (uint16_t k = 0; k < (*itRlcPdu).second.size (); k++)
                {
                  (*itRlcPdu).second.at (k).at (harqId).clear ();
                }
              continue;
            }
          // check the feasibility of retransmitting on the same RBGs
          // translate the DCI to Spectrum framework
          std::vector <int> dciRbg;
          uint32_t mask = 0x1;
          NS_LOG_INFO ("Original RBGs " << dci.m_rbBitmap << " rnti " << dci.m_rnti);
          for (int j = 0; j < 32; j++)
            {
              if (((dci.m_rbBitmap & mask) >> j) == 1)
                {
                  dciRbg.push_back (j);
                  NS_LOG_INFO ("\t" << j);
                }
              mask = (mask << 1);
            }
          bool free = true;
          for (uint8_t j = 0; j < dciRbg.size (); j++)
            {
              if (rbgMap.at (dciRbg.at (j)) == true)
                {
                  free = false;
                  break;
                }
            }
          if (free)
            {
              // use the same RBGs for the retx
              // reserve RBGs
              for (uint8_t j = 0; j < dciRbg.size (); j++)
                {
                  rbgMap.at (dciRbg.at (j)) = true;
                  NS_LOG_INFO ("RBG " << dciRbg.at (j) << " assigned");
                  rbgAllocatedNum++;
                }

              NS_LOG_INFO (this << " Send retx in the same RBGs");
            }
          else
            {
              // find RBGs for sending HARQ retx
              uint8_t j = 0;
              uint8_t rbgId = (dciRbg.at (dciRbg.size () - 1) + 1) % numberOfRBGs;
              uint8_t startRbg = dciRbg.at (dciRbg.size () - 1);
              std::vector <bool> rbgMapCopy = rbgMap;
              while ((j < dciRbg.size ())&&(startRbg != rbgId))
                {
                  if (rbgMapCopy.at (rbgId) == false)
                    {
                      rbgMapCopy.at (rbgId) = true;
                      dciRbg.at (j) = rbgId;
                      j++;
                    }
                  rbgId = (rbgId + 1) % numberOfRBGs;
                }
              if (j == dciRbg.size ())
                {
                  // find new RBGs -> update DCI map
                  uint32_t rbgMask = 0;
                  for (uint16_t k = 0; k < dciRbg.size (); k++)
                    {
                      rbgMask = rbgMask + (0x1 << dciRbg.at (k));
                      rbgAllocatedNum++;
                    }
                  dci.m_rbBitmap = rbgMask;
                  rbgMap = rbgMapCopy;
                  NS_LOG_INFO (this << " Move retx in RBGs " << dciRbg.size ());
                }
              else
                {
                  // HARQ retx cannot be performed on this TTI -> store it
                  dlInfoListUntxed.push_back (m_dlInfoListBuffered.at (i));
                  NS_LOG_INFO (this << " No resource for this retx -> buffer it");
                }
            }
          // retrieve RLC PDU list for retx TBsize and update DCI
          BuildDataListElement_s newEl;
          std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (rnti);
          if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << rnti);
            }
          for (uint8_t j = 0; j < nLayers; j++)
            {
              if (retx.at (j))
                {
                  if (j >= dci.m_ndi.size ())
                    {
                      // for avoiding errors in MIMO transient phases
                      dci.m_ndi.push_back (0);
                      dci.m_rv.push_back (0);
                      dci.m_mcs.push_back (0);
                      dci.m_tbsSize.push_back (0);
                      NS_LOG_INFO (this << " layer " << (uint16_t)j << " no txed (MIMO transition)");
                    }
                  else
                    {
                      dci.m_ndi.at (j) = 0;
                      dci.m_rv.at (j)++;
                      (*itHarq).second.at (harqId).m_rv.at (j)++;
                      NS_LOG_INFO (this << " layer " << (uint16_t)j << " RV " << (uint16_t)dci.m_rv.at (j));
                    }
                }
              else
                {
                  // empty TB of layer j
                  dci.m_ndi.at (j) = 0;
                  dci.m_rv.at (j) = 0;
                  dci.m_mcs.at (j) = 0;
                  dci.m_tbsSize.at (j) = 0;
                  NS_LOG_INFO (this << " layer " << (uint16_t)j << " no retx");
                }
            }
          for (uint16_t k = 0; k < (*itRlcPdu).second.at (0).at (dci.m_harqProcess).size (); k++)
            {
              std::vector <struct RlcPduListElement_s> rlcPduListPerLc;
              for (uint8_t j = 0; j < nLayers; j++)
                {
                  if (retx.at (j))
                    {
                      if (j < dci.m_ndi.size ())
                        {
                          NS_LOG_INFO (" layer " << (uint16_t)j << " tb size " << dci.m_tbsSize.at (j));
                          rlcPduListPerLc.push_back ((*itRlcPdu).second.at (j).at (dci.m_harqProcess).at (k));
                        }
                    }
                  else
                    { // if no retx needed on layer j, push an RlcPduListElement_s object with m_size=0 to keep the size of rlcPduListPerLc vector = 2 in case of MIMO
                      NS_LOG_INFO (" layer " << (uint16_t)j << " tb size "<<dci.m_tbsSize.at (j));
                      RlcPduListElement_s emptyElement;
                      emptyElement.m_logicalChannelIdentity = (*itRlcPdu).second.at (j).at (dci.m_harqProcess).at (k).m_logicalChannelIdentity;
                      emptyElement.m_size = 0;
                      rlcPduListPerLc.push_back (emptyElement);
                    }
                }

              if (rlcPduListPerLc.size () > 0)
                {
                  newEl.m_rlcPduList.push_back (rlcPduListPerLc);
                }
            }
          newEl.m_rnti = rnti;
          newEl.m_dci = dci;
          (*itHarq).second.at (harqId).m_rv = dci.m_rv;
          // refresh timer
          std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer = m_dlHarqProcessesTimer.find (rnti);
          if (itHarqTimer== m_dlHarqProcessesTimer.end ())
            {
              NS_FATAL_ERROR ("Unable to find HARQ timer for RNTI " << (uint16_t)rnti);
            }
          (*itHarqTimer).second.at (harqId) = 0;
          ret.m_buildDataList.push_back (newEl);
          rntiAllocated.insert (rnti);
        }
      else
        {
          // update HARQ process status
          NS_LOG_INFO (this << " HARQ received ACK for UE " << m_dlInfoListBuffered.at (i).m_rnti);
          std::map <uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find (m_dlInfoListBuffered.at (i).m_rnti);
          if (it == m_dlHarqProcessesStatus.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << m_dlInfoListBuffered.at (i).m_rnti);
            }
          (*it).second.at (m_dlInfoListBuffered.at (i).m_harqProcessId) = 0;
          std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (m_dlInfoListBuffered.at (i).m_rnti);
          if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at (i).m_rnti);
            }
          for (uint16_t k = 0; k < (*itRlcPdu).second.size (); k++)
            {
              (*itRlcPdu).second.at (k).at (m_dlInfoListBuffered.at (i).m_harqProcessId).clear ();
            }
        }
    }

  m_dlInfoListBuffered.clear ();
  m_dlInfoListBuffered = dlInfoListUntxed;

  if (rbgAllocatedNum == numberOfRBGs)
    {
      // all the RBGs are already allocated -> exit
      if ((ret.m_buildDataList.size () > 0) || (ret.m_buildRarList.size () > 0))
        {
          m_schedSapUser->SchedDlConfigInd (ret);
        }
      return;
    }

  std::map <LteFlowId_t,struct LogicalChannelConfigListElement_s>::iterator itLogicalChannels;
	
  for (itLogicalChannels = m_ueLogicalChannelsConfigList.begin (); itLogicalChannels != m_ueLogicalChannelsConfigList.end (); itLogicalChannels++)
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find (itLogicalChannels->first.m_rnti);
      if ((itRnti != rntiAllocated.end ())||(!HarqProcessAvailability (itLogicalChannels->first.m_rnti)))
        {
          // UE already allocated for HARQ or without HARQ process available -> drop it
          if (itRnti != rntiAllocated.end ())
            {
              NS_LOG_DEBUG (this << " RNTI discared for HARQ tx" << (uint16_t)(itLogicalChannels->first.m_rnti));
            }
          if (!HarqProcessAvailability (itLogicalChannels->first.m_rnti))
            {
              NS_LOG_DEBUG (this << " RNTI discared for HARQ id" << (uint16_t)(itLogicalChannels->first.m_rnti));
            }
          continue;
        }


      std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator itRlcBufferReq = m_rlcBufferReq_1.find (itLogicalChannels->first);
      if (itRlcBufferReq==m_rlcBufferReq_1.end ())
        continue;

      int group = -1;
      int delay = 0;

      if (itRlcBufferReq->second.m_rlcRetransmissionQueueSize > 0)
        {
          delay = itRlcBufferReq->second.m_rlcRetransmissionHolDelay;
          group = delay/grouping_parameter;
        }
      else if  (itRlcBufferReq->second.m_rlcTransmissionQueueSize > 0)
        {
          delay = itRlcBufferReq->second.m_rlcTransmissionQueueHolDelay;
          group = delay/grouping_parameter;
        }
      else
        {
          continue;
        }

      UEtoHOL.insert (std::pair<LteFlowId_t,int>(itLogicalChannels->first,delay));

      if (itLogicalChannels->second.m_qosBearerType == itLogicalChannels->second.QBT_NON_GBR )
        {
          if (map_nonGBRHOLgroupToUE.count (group)==0)
            {
              std::set<LteFlowId_t> v;
              v.insert (itRlcBufferReq->first);
              map_nonGBRHOLgroupToUE.insert (std::pair<int,std::set<LteFlowId_t> >(group,v));
            }
          else
            {
              map_nonGBRHOLgroupToUE.find (group)->second.insert (itRlcBufferReq->first);
            }
        }
      else if (itLogicalChannels->second.m_qosBearerType == itLogicalChannels->second.QBT_GBR) {
          if (map_GBRHOLgroupToUE.count (group)==0)
            {
              std::set<LteFlowId_t> v;
              v.insert (itRlcBufferReq->first);
              map_GBRHOLgroupToUE.insert (std::pair<int,std::set<LteFlowId_t> >(group,v));
            }
          else
            {
              map_GBRHOLgroupToUE.find (group)->second.insert (itRlcBufferReq->first);
            }
        }
    };


  // Prepare data for the scheduling mechanism
  // map: UE, to the amount of traffic they have to transfer
  std::map<LteFlowId_t, int> UeToAmountOfDataToTransfer;
  //Initialize the map per UE, how much resources is already assigned to the user
  std::map<LteFlowId_t, int> UeToAmountOfAssignedResources;
  // prepare values to calculate FF metric, this metric will be the same for all flows(logical channels) that belong to the same RNTI
  std::map < uint16_t, uint8_t > sbCqiSum;

  for( std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator itrbr = m_rlcBufferReq_1.begin ();
       itrbr!=m_rlcBufferReq_1.end (); itrbr++)
    {

      LteFlowId_t flowId = itrbr->first;                // Prepare data for the scheduling mechanism
      // check first the channel conditions for this UE, if CQI!=0
      std::map <uint16_t,SbMeasResult_s>::iterator itCqi;
      itCqi = m_a30CqiRxed.find ((*itrbr).first.m_rnti);
      std::map <uint16_t,uint8_t>::iterator itTxMode;
      itTxMode = m_uesTxMode.find ((*itrbr).first.m_rnti);
      if (itTxMode == m_uesTxMode.end ())
        {
          NS_FATAL_ERROR ("No Transmission Mode info on user " << (*itrbr).first.m_rnti);
        }
      int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);

      uint8_t cqiSum = 0;
      for (int k = 0; k < numberOfRBGs; k++)
        {
          for (uint8_t j = 0; j < nLayer; j++)
            {
              if (itCqi == m_a30CqiRxed.end ())
                {
                  cqiSum += 1;  // no info on this user -> lowest MCS
                }
              else
                {
                  cqiSum += (*itCqi).second.m_higherLayerSelected.at (k).m_sbCqi.at(j);
                }
            }
        }

      if (cqiSum == 0)
        {
          NS_LOG_INFO ("Skip this flow, CQI==0, rnti:"<<(*itrbr).first.m_rnti);
          continue;
        }
      
      // map: UE, to the amount of traffic they have to transfer
      int amountOfDataToTransfer =  8*((int)m_rlcBufferReq_1.find (flowId)->second.m_rlcRetransmissionQueueSize +
                                       (int)m_rlcBufferReq_1.find (flowId)->second.m_rlcTransmissionQueueSize);

      UeToAmountOfDataToTransfer.insert (std::pair<LteFlowId_t,int>(flowId,amountOfDataToTransfer));
      UeToAmountOfAssignedResources.insert (std::pair<LteFlowId_t,int>(flowId,0));

      uint8_t sum = 0;
      for (int i = 0; i < numberOfRBGs; i++)
        {
          std::map <uint16_t,SbMeasResult_s>::iterator itCqi;
          itCqi = m_a30CqiRxed.find ((*itrbr).first.m_rnti);
          std::map <uint16_t,uint8_t>::iterator itTxMode;
          itTxMode = m_uesTxMode.find ((*itrbr).first.m_rnti);
          if (itTxMode == m_uesTxMode.end ())
            {
              NS_FATAL_ERROR ("No Transmission Mode info on user " << (*itrbr).first.m_rnti);
            }
          int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);
          std::vector <uint8_t> sbCqis;
          if (itCqi == m_a30CqiRxed.end ())
            {
              for (uint8_t k = 0; k < nLayer; k++)
                {
                  sbCqis.push_back (1);                        // start with lowest value
                }
            }
          else
            {
              sbCqis = (*itCqi).second.m_higherLayerSelected.at (i).m_sbCqi;
            }

          uint8_t cqi1 = sbCqis.at (0);
          uint8_t cqi2 = 0;
          if (sbCqis.size () > 1)
            {
              cqi2 = sbCqis.at (1);
            }

          uint8_t sbCqi = 0;
          if ((cqi1 > 0)||(cqi2 > 0))               // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
            {
              for (uint8_t k = 0; k < nLayer; k++)
                {
                  if (sbCqis.size () > k)
                    {
                      sbCqi = sbCqis.at (k);
                    }
                  else
                    {
                      // no info on this subband
                      sbCqi = 0;
                    }
                  sum += sbCqi;
                }
            }               // end if cqi
        }        // end of rbgNum

      sbCqiSum.insert (std::pair<uint16_t, uint8_t> ((*itrbr).first.m_rnti, sum));
    }

  // availableRBGs - set that contains indexes of available resource block groups
  std::set<int> availableRBGs;
  for (int i = 0; i <  numberOfRBGs; i++)
    {
      if (rbgMap.at (i) == false)
        {
          availableRBGs.insert (i);
        }
    }

  t_it_HOLgroupToUEs itGBRgroups = map_GBRHOLgroupToUE.begin ();
  t_it_HOLgroupToUEs itnonGBRgroups = map_nonGBRHOLgroupToUE.begin ();



  // while there are more resources available, loop through the users that are grouped by HOL value
  while (availableRBGs.size ()>0)
    {
      if (UeToAmountOfDataToTransfer.size() == 0)
        {
          NS_LOG_INFO ("No UEs to be scheduled (no data or CQI==0),");
          break;
        }
      std::set<LteFlowId_t> vUEs;
      t_it_HOLgroupToUEs itCurrentGroup;

      if (itGBRgroups!=map_GBRHOLgroupToUE.end ())
        {
          itCurrentGroup=itGBRgroups;
          itGBRgroups++;
        }
      else if (itnonGBRgroups!=map_nonGBRHOLgroupToUE.end ())                  // if there are no more flows with retransmission queue start to scheduler flows with transmission queue
        {
          itCurrentGroup=itnonGBRgroups;
          itnonGBRgroups++;
        }
      else
        {
          NS_LOG_INFO ("Available RBGs:"<< availableRBGs.size ()<<"but no users");
          break;
        }

      while (availableRBGs.size ()>0 and itCurrentGroup->second.size ()>0)
        {
          bool currentRBchecked = false;
          int currentRB = *(availableRBGs.begin ());
          std::map<LteFlowId_t, CQI_value> UeToCQIValue;
          std::map<LteFlowId_t, double > UeToCoitaMetric;
          std::map<LteFlowId_t, bool> UeHasReachedGBR;
          double maximumValueMetric = 0;
          LteFlowId_t userWithMaximumMetric;
          UeToCQIValue.clear ();
          UeToCoitaMetric.clear ();

          // Iterate through the users and calculate which user will use the best of the current resource bloc.end()k and assign to that user.
          for (std::set<LteFlowId_t>::iterator it=itCurrentGroup->second.begin (); it!=itCurrentGroup->second.end (); it++)
            {
              userWithMaximumMetric = *it;
              LteFlowId_t flowId = *it;
              uint8_t cqi_value = 1;                           //higher better, maximum is 15
              double coita_metric = 1;
              double coita_sum = 0;
              double metric = 0;
              uint8_t worstCQIAmongRBGsAllocatedForThisUser = 15;
              int numberOfRBGAllocatedForThisUser = 0;
              LogicalChannelConfigListElement_s lc = m_ueLogicalChannelsConfigList.find (flowId)->second;
              std::map <uint16_t,SbMeasResult_s>::iterator itRntiCQIsMap = m_a30CqiRxed.find (flowId.m_rnti);

              std::map <uint16_t, RrsFlowPerf_t>::iterator itStats;

              if ((m_ffrSapProvider->IsDlRbgAvailableForUe (currentRB, flowId.m_rnti)) == false)
                {
                  continue;
                }

              if (m_flowStatsDl.find (flowId.m_rnti) == m_flowStatsDl.end ())
                {
                  continue;                               // TO DO:  check if this should be logged and how.
                }
              currentRBchecked = true;

              itStats = m_flowStatsDl.find (flowId.m_rnti);
              double tbr_weight = (*itStats).second.targetThroughput / (*itStats).second.lastAveragedThroughput;
              if (tbr_weight < 1.0)
                tbr_weight = 1.0;

              if (itRntiCQIsMap != m_a30CqiRxed.end ())
                {
                  for(std::set<int>::iterator it=availableRBGs.begin (); it!=availableRBGs.end (); it++)
                    {
                      try
                        {
                          int val = (itRntiCQIsMap->second.m_higherLayerSelected.at (*it).m_sbCqi.at (0));
                          if (val==0)
                            val=1;                                             //if no info, use minimum
                          if (*it == currentRB)
                            cqi_value = val;
                          coita_sum+=val;

                        }
                      catch(std::out_of_range&)
                        {
                          coita_sum+=1;                                      //if no info on channel use the worst cqi
                          NS_LOG_INFO ("No CQI for lcId:"<<flowId.m_lcId<<" rnti:"<<flowId.m_rnti<<" at subband:"<<currentRB);
                          //std::cout<<"\n No CQI for lcId:.....................................";
                        }
                    }
                  coita_metric =cqi_value/coita_sum;
                  UeToCQIValue.insert (std::pair<LteFlowId_t,CQI_value>(flowId,cqi_value));
                  UeToCoitaMetric.insert (std::pair<LteFlowId_t, double>(flowId,coita_metric));
                }

              if (allocationMapPerRntiPerLCId.find (flowId.m_rnti)==allocationMapPerRntiPerLCId.end ())
                {
                  worstCQIAmongRBGsAllocatedForThisUser=cqi_value;
                }
              else {

                  numberOfRBGAllocatedForThisUser = (allocationMapPerRntiPerLCId.find (flowId.m_rnti)->second.size ());

                  for (std::multimap <uint8_t, qos_rb_and_CQI_assigned_to_lc>::iterator itRBG = allocationMapPerRntiPerLCId.find (flowId.m_rnti)->second.begin ();
                       itRBG!=allocationMapPerRntiPerLCId.find (flowId.m_rnti)->second.end (); itRBG++)
                    {
                      qos_rb_and_CQI_assigned_to_lc e = itRBG->second;
                      if (e.cqi_value_for_lc < worstCQIAmongRBGsAllocatedForThisUser)
                        worstCQIAmongRBGsAllocatedForThisUser=e.cqi_value_for_lc;
                    }

                  if (cqi_value < worstCQIAmongRBGsAllocatedForThisUser)
                    {
                      worstCQIAmongRBGsAllocatedForThisUser=cqi_value;
                    }
                }

              int mcsForThisUser = m_amc->GetMcsFromCqi (worstCQIAmongRBGsAllocatedForThisUser);
              int tbSize = m_amc->GetDlTbSizeFromMcs (mcsForThisUser, (numberOfRBGAllocatedForThisUser+1) * rbgSize)/8;                           // similar to calculation of TB size (size of TB in bytes according to table 7.1.7.2.1-1 of 36.213)


              double achievableRate = (( m_amc->GetDlTbSizeFromMcs (mcsForThisUser, rbgSize)/ 8) / 0.001);
              double pf_weight = achievableRate / (*itStats).second.secondLastAveragedThroughput;

              UeToAmountOfAssignedResources.find (flowId)->second = 8*tbSize;
              FfMacSchedSapProvider::SchedDlRlcBufferReqParameters lcBufferInfo = m_rlcBufferReq_1.find (flowId)->second;

              if (UeToAmountOfDataToTransfer.find (flowId)->second - UeToAmountOfAssignedResources.find (flowId)->second < 0)
                {
                  UeHasReachedGBR.insert (std::pair<LteFlowId_t,bool>(flowId,false));
                }

              double bitRateWithNewRBG = 0;

              if (m_flowStatsDl.find (flowId.m_rnti)!= m_flowStatsDl.end ())                         // there are some statistics{
                {
                  bitRateWithNewRBG = (1.0 - (1.0 / m_timeWindow)) * (m_flowStatsDl.find (flowId.m_rnti)->second.lastAveragedThroughput) + ((1.0 / m_timeWindow) * (double)(tbSize*1000));
                }
              else
                {
                  bitRateWithNewRBG = (1.0 / m_timeWindow) * (double)(tbSize*1000);
                }

              if(bitRateWithNewRBG > lc.m_eRabGuaranteedBitrateDl)
                {
                  UeHasReachedGBR.insert (std::pair<LteFlowId_t,bool>(flowId,true));
                }
              else
                {
                  UeHasReachedGBR.insert (std::pair<LteFlowId_t,bool>(flowId,false));
                }

              int hol = UEtoHOL.find (flowId)->second;

              if (hol==0)
                hol=1;

              if ( m_CqaMetric.compare ("CqaFf") == 0)
                {
                  metric = coita_metric*tbr_weight*hol;
                }
              else if (m_CqaMetric.compare ("CqaPf") == 0)
                {
                  metric = tbr_weight*pf_weight*hol;
                }
              else
                {
                  metric = 1;
                }

              if (metric >= maximumValueMetric)
                {
                  maximumValueMetric = metric;
                  userWithMaximumMetric = flowId;
                }
            }

          if (!currentRBchecked)
            {
              // erase current RBG from the list of available RBG
              availableRBGs.erase (currentRB);
              continue;
            }

          qos_rb_and_CQI_assigned_to_lc s;
          s.cqi_value_for_lc = UeToCQIValue.find (userWithMaximumMetric)->second;
          s.resource_block_index = currentRB;

          itMap = allocationMapPerRntiPerLCId.find (userWithMaximumMetric.m_rnti);

          if (itMap == allocationMapPerRntiPerLCId.end ())
            {
              std::multimap <uint8_t, qos_rb_and_CQI_assigned_to_lc> tempMap;
              tempMap.insert (std::pair<uint8_t, qos_rb_and_CQI_assigned_to_lc> (userWithMaximumMetric.m_lcId,s));
              allocationMapPerRntiPerLCId.insert (std::pair <uint16_t, std::multimap <uint8_t,qos_rb_and_CQI_assigned_to_lc> > (userWithMaximumMetric.m_rnti, tempMap));
            }
          else
            {
              itMap->second.insert (std::pair<uint8_t,qos_rb_and_CQI_assigned_to_lc> (userWithMaximumMetric.m_lcId,s));
            }

          // erase current RBG from the list of available RBG
          availableRBGs.erase (currentRB);

          if (UeToAmountOfDataToTransfer.find (userWithMaximumMetric)->second <= UeToAmountOfAssignedResources.find (userWithMaximumMetric)->second*tolerance)
          //||(UeHasReachedGBR.find(userWithMaximumMetric)->second == true))
            {
              itCurrentGroup->second.erase (userWithMaximumMetric);
            }

        }                 // while there are more users in current group
    }             // while there are more groups of users


  // reset TTI stats of users
  std::map <uint16_t, RrsFlowPerf_t>::iterator itStats;
  for (itStats = m_flowStatsDl.begin (); itStats != m_flowStatsDl.end (); itStats++)
    {
      (*itStats).second.lastTtiBytesTransmitted = 0;
    }

  // 3) Creating the correspondent DCIs (Generate the transmission opportunities by grouping the RBGs of the same RNTI)
  //FfMacSchedSapUser::SchedDlConfigIndParameters ret;
  itMap = allocationMapPerRntiPerLCId.begin ();
  int counter = 0;
  std::map<uint16_t, double> m_rnti_per_ratio;

  while (itMap != allocationMapPerRntiPerLCId.end ())
    {

      // create new BuildDataListElement_s for this LC
      BuildDataListElement_s newEl;
      newEl.m_rnti = (*itMap).first;
      NS_LOG_INFO ("Scheduled RNTI:"<<newEl.m_rnti);
      // create the DlDciListElement_s
      DlDciListElement_s newDci;
      std::vector <struct RlcPduListElement_s> newRlcPduLe;
      newDci.m_rnti = (*itMap).first;

      // for(std::map <uint16_t, std::multimap <uint8_t, qos_rb_and_CQI_assigned_to_lc> >::iterator zz = allocationMapPerRntiPerLCId.begin();zz != allocationMapPerRntiPerLCId.end(); zz++)
      // {
      //   std::cout << zz->first << std::endl; 
      // }
      // std::cout << "zzzzzzzzzzzzzzzzzzzzz" << std::endl; 
      newDci.m_harqProcess = UpdateHarqProcessId ((*itMap).first);
      // std::cout << "执行结束------1" << std::endl;
      uint16_t lcActives = LcActivePerFlow (itMap->first);
      if (lcActives==0)           // if there is still no buffer report information on any flow
        lcActives = 1;
      // NS_LOG_DEBUG (this << "Allocate user " << newEl.m_rnti << " rbg " << lcActives);
      // std::cout << "执行结束------1" << std::endl;
      uint16_t RgbPerRnti = (*itMap).second.size ();
      double doubleRBgPerRnti = RgbPerRnti;
      double doubleRbgNum = numberOfRBGs;
      double rrRatio = doubleRBgPerRnti/doubleRbgNum;
      m_rnti_per_ratio.insert (std::pair<uint16_t,double>((*itMap).first,rrRatio));
      std::map <uint16_t,SbMeasResult_s>::iterator itCqi;
      itCqi = m_a30CqiRxed.find ((*itMap).first);
      uint8_t worstCqi = 15;
      // std::cout << "执行结束------1" << std::endl;
      // assign the worst value of CQI that user experienced on any of its subbands
      for ( std::multimap <uint8_t, qos_rb_and_CQI_assigned_to_lc> ::iterator it = (*itMap).second.begin (); it != (*itMap).second.end (); it++)
        {
          if (it->second.cqi_value_for_lc<worstCqi)
            {
              worstCqi = it->second.cqi_value_for_lc;
            }
          counter++;
        }
      // std::cout << "执行结束------1" << std::endl;
      newDci.m_mcs.push_back (m_amc->GetMcsFromCqi (worstCqi));
      int tbSize = (m_amc->GetDlTbSizeFromMcs (newDci.m_mcs.at (0), RgbPerRnti * rbgSize) / 8);           // (size of TB in bytes according to table 7.1.7.2.1-1 of 36.213)
      newDci.m_tbsSize.push_back (tbSize);
      newDci.m_resAlloc = 0;            // only allocation type 0 at this stage
      newDci.m_rbBitmap = 0;    // TBD (32 bit bitmap see 7.1.6 of 36.213)
      uint32_t rbgMask = 0;
      std::multimap <uint8_t, qos_rb_and_CQI_assigned_to_lc> ::iterator itRBGsPerRNTI;
      for ( itRBGsPerRNTI = (*itMap).second.begin (); itRBGsPerRNTI != (*itMap).second.end (); itRBGsPerRNTI++)
        {
          rbgMask = rbgMask + (0x1 << itRBGsPerRNTI->second.resource_block_index);
        }
      newDci.m_rbBitmap = rbgMask;   // (32 bit bitmap see 7.1.6 of 36.213)
      // NOTE: In this first version of CqaFfMacScheduler, it is assumed one flow per user.
      // create the rlc PDUs -> equally divide resources among active LCs
      std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator itBufReq;
      for (itBufReq = m_rlcBufferReq_1.begin (); itBufReq != m_rlcBufferReq_1.end (); itBufReq++)
        {
          if (((*itBufReq).first.m_rnti == (*itMap).first)
              && (((*itBufReq).second.m_rlcTransmissionQueueSize > 0)
                  || ((*itBufReq).second.m_rlcRetransmissionQueueSize > 0)
                  || ((*itBufReq).second.m_rlcStatusPduSize > 0) ))
            {
              std::vector <struct RlcPduListElement_s> newRlcPduLe;
              //for (uint8_t j = 0; j < nLayer; j++)
              //{
              RlcPduListElement_s newRlcEl;
              newRlcEl.m_logicalChannelIdentity = (*itBufReq).first.m_lcId;
              // newRlcEl.m_size = newDci.m_tbsSize.at (j) / lcActives;
              newRlcEl.m_size = tbSize / lcActives;
              // NS_LOG_INFO (this << " LCID " << (uint32_t) newRlcEl.m_logicalChannelIdentity << " size " << newRlcEl.m_size << " layer " << (uint16_t)j);
              newRlcPduLe.push_back (newRlcEl);
              UpdateDlRlcBufferInfo (newDci.m_rnti, newRlcEl.m_logicalChannelIdentity, newRlcEl.m_size);
              if (m_harqOn == true)
                {
                  // store RLC PDU list for HARQ
                  std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find ((*itMap).first);
                  if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
                    {
                      NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << (*itMap).first);
                    }
                  int j=0;
                  (*itRlcPdu).second.at (j).at (newDci.m_harqProcess).push_back (newRlcEl);
                }
              // }
              newEl.m_rlcPduList.push_back (newRlcPduLe);
            }
          if ((*itBufReq).first.m_rnti > (*itMap).first)
            {
              break;
            }
        }
      // for (uint8_t j = 0; j < nLayer; j++)
      // {
      newDci.m_ndi.push_back (1);
      newDci.m_rv.push_back (0);
      //}

      newDci.m_tpc = m_ffrSapProvider->GetTpc ((*itMap).first);

      newEl.m_dci = newDci;

      if (m_harqOn == true)
        {
          // store DCI for HARQ
          std::map <uint16_t, DlHarqProcessesDciBuffer_t>::iterator itDci = m_dlHarqProcessesDciBuffer.find (newEl.m_rnti);
          if (itDci == m_dlHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in DCI HARQ buffer for RNTI " << newEl.m_rnti);
            }
          (*itDci).second.at (newDci.m_harqProcess) = newDci;
          // refresh timer
          std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer =  m_dlHarqProcessesTimer.find (newEl.m_rnti);
          if (itHarqTimer== m_dlHarqProcessesTimer.end ())
            {
              NS_FATAL_ERROR ("Unable to find HARQ timer for RNTI " << (uint16_t)newEl.m_rnti);
            }
          (*itHarqTimer).second.at (newDci.m_harqProcess) = 0;
        }

      // ...more parameters -> ignored in this version

      ret.m_buildDataList.push_back (newEl);
      // update UE stats
      std::map <uint16_t, RrsFlowPerf_t>::iterator it;
      it = m_flowStatsDl.find ((*itMap).first);
      if (it != m_flowStatsDl.end ())
        {
          (*it).second.lastTtiBytesTransmitted = tbSize;
        }
      else
        {
          NS_FATAL_ERROR (this << " No Stats for this allocated UE");
        }

      itMap++;
    } // end while allocation
  ret.m_nrOfPdcchOfdmSymbols = 1;   // TODO: check correct value according the DCIs txed
  // std::cout << "执行结束------" << std::endl;
  // update UEs stats
  NS_LOG_INFO (this << " Update UEs statistics");
  for (itStats = m_flowStatsDl.begin (); itStats != m_flowStatsDl.end (); itStats++)
    {
      if (allocationMapPerRntiPerLCId.find (itStats->first)!= allocationMapPerRntiPerLCId.end ())
        {
          (*itStats).second.secondLastAveragedThroughput = ((1.0 - (1 / m_timeWindow)) * (*itStats).second.secondLastAveragedThroughput) + ((1 / m_timeWindow) * (double)((*itStats).second.lastTtiBytesTransmitted / 0.001));
        }

      (*itStats).second.totalBytesTransmitted += (*itStats).second.lastTtiBytesTransmitted;
      // update average throughput (see eq. 12.3 of Sec 12.3.1.2 of LTE – The UMTS Long Term Evolution, Ed Wiley)
      (*itStats).second.lastAveragedThroughput = ((1.0 - (1.0 / m_timeWindow)) * (*itStats).second.lastAveragedThroughput) + ((1.0 / m_timeWindow) * (double)((*itStats).second.lastTtiBytesTransmitted / 0.001));
      NS_LOG_INFO (this << " UE total bytes " << (*itStats).second.totalBytesTransmitted);
      NS_LOG_INFO (this << " UE average throughput " << (*itStats).second.lastAveragedThroughput);
      (*itStats).second.lastTtiBytesTransmitted = 0;
    }

  m_schedSapUser->SchedDlConfigInd (ret);
  // std::cout << "执行结束------" << std::endl;
  int count_allocated_resource_blocks = 0;
  for (std::map <uint16_t, std::multimap <uint8_t, qos_rb_and_CQI_assigned_to_lc> >::iterator itMap = allocationMapPerRntiPerLCId.begin (); itMap!=allocationMapPerRntiPerLCId.end (); itMap++)
    {
      count_allocated_resource_blocks+=itMap->second.size ();
    }
  NS_LOG_INFO (this << " Allocated RBs:" << count_allocated_resource_blocks);
  // std::cout << "执行结束" << std::endl;
  return;
  }
}

void
RrFfMacScheduler::DoSchedDlRachInfoReq (const struct FfMacSchedSapProvider::SchedDlRachInfoReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  
  m_rachList = params.m_rachList;

  return;
}

void
RrFfMacScheduler::DoSchedDlCqiInfoReq (const struct FfMacSchedSapProvider::SchedDlCqiInfoReqParameters& params)
{
  // if(m_flag == 0)
  // {
  //   std::cout << "执行0" << std::endl;
  
  // NS_LOG_FUNCTION (this);

  // std::map <uint16_t,uint8_t>::iterator it;
  // for (unsigned int i = 0; i < params.m_cqiList.size (); i++)
  //   {
  //     if ( params.m_cqiList.at (i).m_cqiType == CqiListElement_s::P10 )
  //       {
  //         NS_LOG_LOGIC ("wideband CQI " <<  (uint32_t) params.m_cqiList.at (i).m_wbCqi.at (0) << " reported");
  //         std::map <uint16_t,uint8_t>::iterator it;
  //         uint16_t rnti = params.m_cqiList.at (i).m_rnti;
  //         it = m_p10CqiRxed.find (rnti);
  //         if (it == m_p10CqiRxed.end ())
  //           {
  //             // create the new entry
  //             m_p10CqiRxed.insert ( std::pair<uint16_t, uint8_t > (rnti, params.m_cqiList.at (i).m_wbCqi.at (0)) ); // only codeword 0 at this stage (SISO)
  //             // generate correspondent timer
  //             m_p10CqiTimers.insert ( std::pair<uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
  //           }
  //         else
  //           {
  //             // update the CQI value
  //             (*it).second = params.m_cqiList.at (i).m_wbCqi.at (0);
  //             // update correspondent timer
  //             std::map <uint16_t,uint32_t>::iterator itTimers;
  //             itTimers = m_p10CqiTimers.find (rnti);
  //             (*itTimers).second = m_cqiTimersThreshold;
  //           }
  //       }
  //     else if ( params.m_cqiList.at (i).m_cqiType == CqiListElement_s::A30 )
  //       {
  //         // subband CQI reporting high layer configured
  //         // Not used by RR Scheduler
  //       }
  //     else
  //       {
  //         NS_LOG_ERROR (this << " CQI type unknown");
  //       }
  //   }

  // return;
  // }
  // else if(m_flag == 2)
  // {
  //   NS_LOG_FUNCTION (this);

  // for (unsigned int i = 0; i < params.m_cqiList.size (); i++)
  //   {
  //     if ( params.m_cqiList.at (i).m_cqiType == CqiListElement_s::P10 )
  //       {
  //         NS_LOG_LOGIC ("wideband CQI " <<  (uint32_t) params.m_cqiList.at (i).m_wbCqi.at (0) << " reported");
  //         std::map <uint16_t,uint8_t>::iterator it;
  //         uint16_t rnti = params.m_cqiList.at (i).m_rnti;
  //         it = m_p10CqiRxed.find (rnti);
  //         if (it == m_p10CqiRxed.end ())
  //           {
  //             // create the new entry
  //             m_p10CqiRxed.insert ( std::pair<uint16_t, uint8_t > (rnti, params.m_cqiList.at (i).m_wbCqi.at (0)) ); // only codeword 0 at this stage (SISO)
  //             // generate correspondent timer
  //             m_p10CqiTimers.insert ( std::pair<uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
  //           }
  //         else
  //           {
  //             // update the CQI value and refresh correspondent timer
  //             (*it).second = params.m_cqiList.at (i).m_wbCqi.at (0);
  //             // update correspondent timer
  //             std::map <uint16_t,uint32_t>::iterator itTimers;
  //             itTimers = m_p10CqiTimers.find (rnti);
  //             (*itTimers).second = m_cqiTimersThreshold;
  //           }
  //       }
  //     else if ( params.m_cqiList.at (i).m_cqiType == CqiListElement_s::A30 )
  //       {
  //         // subband CQI reporting high layer configured
  //         std::map <uint16_t,SbMeasResult_s>::iterator it;
  //         uint16_t rnti = params.m_cqiList.at (i).m_rnti;
  //         it = m_a30CqiRxed.find (rnti);
  //         if (it == m_a30CqiRxed.end ())
  //           {
  //             // create the new entry
  //             m_a30CqiRxed.insert ( std::pair<uint16_t, SbMeasResult_s > (rnti, params.m_cqiList.at (i).m_sbMeasResult) );
  //             m_a30CqiTimers.insert ( std::pair<uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
  //           }
  //         else
  //           {
  //             // update the CQI value and refresh correspondent timer
  //             (*it).second = params.m_cqiList.at (i).m_sbMeasResult;
  //             std::map <uint16_t,uint32_t>::iterator itTimers;
  //             itTimers = m_a30CqiTimers.find (rnti);
  //             (*itTimers).second = m_cqiTimersThreshold;
  //           }
  //       }
  //     else
  //       {
  //         NS_LOG_ERROR (this << " CQI type unknown");
  //       }
  //   }

  // return;
  // }
  // else if(m_flag == 3)
  // {

  // NS_LOG_FUNCTION (this);
  // m_ffrSapProvider->ReportDlCqiInfo (params);

  // for (unsigned int i = 0; i < params.m_cqiList.size (); i++)
  //   {
  //     if ( params.m_cqiList.at (i).m_cqiType == CqiListElement_s::P10 )
  //       {
  //         NS_LOG_LOGIC ("wideband CQI " <<  (uint32_t) params.m_cqiList.at (i).m_wbCqi.at (0) << " reported");
  //         std::map <uint16_t,uint8_t>::iterator it;
  //         uint16_t rnti = params.m_cqiList.at (i).m_rnti;
  //         it = m_p10CqiRxed.find (rnti);
  //         if (it == m_p10CqiRxed.end ())
  //           {
  //             // create the new entry
  //             m_p10CqiRxed.insert ( std::pair<uint16_t, uint8_t > (rnti, params.m_cqiList.at (i).m_wbCqi.at (0)) ); // only codeword 0 at this stage (SISO)
  //             // generate correspondent timer
  //             m_p10CqiTimers.insert ( std::pair<uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
  //           }
  //         else
  //           {
  //             // update the CQI value and refresh correspondent timer
  //             (*it).second = params.m_cqiList.at (i).m_wbCqi.at (0);
  //             // update correspondent timer
  //             std::map <uint16_t,uint32_t>::iterator itTimers;
  //             itTimers = m_p10CqiTimers.find (rnti);
  //             (*itTimers).second = m_cqiTimersThreshold;
  //           }
  //       }
  //     else if ( params.m_cqiList.at (i).m_cqiType == CqiListElement_s::A30 )
  //       {
  //         // subband CQI reporting high layer configured
  //         std::map <uint16_t,SbMeasResult_s>::iterator it;
  //         uint16_t rnti = params.m_cqiList.at (i).m_rnti;
  //         it = m_a30CqiRxed.find (rnti);
  //         if (it == m_a30CqiRxed.end ())
  //           {
  //             // create the new entry
  //             m_a30CqiRxed.insert ( std::pair<uint16_t, SbMeasResult_s > (rnti, params.m_cqiList.at (i).m_sbMeasResult) );
  //             m_a30CqiTimers.insert ( std::pair<uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
  //           }
  //         else
  //           {
  //             // update the CQI value and refresh correspondent timer
  //             (*it).second = params.m_cqiList.at (i).m_sbMeasResult;
  //             std::map <uint16_t,uint32_t>::iterator itTimers;
  //             itTimers = m_a30CqiTimers.find (rnti);
  //             (*itTimers).second = m_cqiTimersThreshold;
  //           }
  //       }
  //     else
  //       {
  //         NS_LOG_ERROR (this << " CQI type unknown");
  //       }
  //   }

  // return;
  // }
  // else
  // {
    // std::cout << "执行1" << std::endl;
    NS_LOG_FUNCTION (this);
  m_ffrSapProvider->ReportDlCqiInfo (params);

  for (unsigned int i = 0; i < params.m_cqiList.size (); i++)
    {
      if ( params.m_cqiList.at (i).m_cqiType == CqiListElement_s::P10 )
        {
          NS_LOG_LOGIC ("wideband CQI " <<  (uint32_t) params.m_cqiList.at (i).m_wbCqi.at (0) << " reported");
          std::map <uint16_t,uint8_t>::iterator it;
          uint16_t rnti = params.m_cqiList.at (i).m_rnti;
          it = m_p10CqiRxed.find (rnti);
          if (it == m_p10CqiRxed.end ())
            {
              // create the new entry
              m_p10CqiRxed.insert ( std::pair<uint16_t, uint8_t > (rnti, params.m_cqiList.at (i).m_wbCqi.at (0)) ); // only codeword 0 at this stage (SISO)
              // generate correspondent timer
              m_p10CqiTimers.insert ( std::pair<uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
            }
          else
            {
              // update the CQI value and refresh correspondent timer
              (*it).second = params.m_cqiList.at (i).m_wbCqi.at (0);
              // update correspondent timer
              std::map <uint16_t,uint32_t>::iterator itTimers;
              itTimers = m_p10CqiTimers.find (rnti);
              (*itTimers).second = m_cqiTimersThreshold;
            }
        }
      else if ( params.m_cqiList.at (i).m_cqiType == CqiListElement_s::A30 )
        {
          // subband CQI reporting high layer configured
          std::map <uint16_t,SbMeasResult_s>::iterator it;
          uint16_t rnti = params.m_cqiList.at (i).m_rnti;
          it = m_a30CqiRxed.find (rnti);
          if (it == m_a30CqiRxed.end ())
            {
              // create the new entry
              m_a30CqiRxed.insert ( std::pair<uint16_t, SbMeasResult_s > (rnti, params.m_cqiList.at (i).m_sbMeasResult) );
              m_a30CqiTimers.insert ( std::pair<uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
            }
          else
            {
              // update the CQI value and refresh correspondent timer
              (*it).second = params.m_cqiList.at (i).m_sbMeasResult;
              std::map <uint16_t,uint32_t>::iterator itTimers;
              itTimers = m_a30CqiTimers.find (rnti);
              (*itTimers).second = m_cqiTimersThreshold;
            }
        }
      else
        {
          NS_LOG_ERROR (this << " CQI type unknown");
        }
    }

  return;
  // }
}

double
RrFfMacScheduler::EstimateUlSinr (uint16_t rnti, uint16_t rb)
{
  std::map <uint16_t, std::vector <double> >::iterator itCqi = m_ueCqi.find (rnti);
  if (itCqi == m_ueCqi.end ())
    {
      // no cqi info about this UE
      return (NO_SINR);

    }
  else
    {
      // take the average SINR value among the available
      double sinrSum = 0;
      unsigned int sinrNum = 0;
      for (uint32_t i = 0; i < m_cschedCellConfig.m_ulBandwidth; i++)
        {
          double sinr = (*itCqi).second.at (i);
          if (sinr != NO_SINR)
            {
              sinrSum += sinr;
              sinrNum++;
            }
        }
      double estimatedSinr = (sinrNum > 0) ? (sinrSum / sinrNum) : DBL_MAX;
      // store the value
      (*itCqi).second.at (rb) = estimatedSinr;
      return (estimatedSinr);
    }
}

void
RrFfMacScheduler::DoSchedUlTriggerReq (const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params)
{
  if(m_flag == 0)
  {
    // std::cout << "执行0" << std::endl;
  NS_LOG_FUNCTION (this << " UL - Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf) << " size " << params.m_ulInfoList.size ());

  RefreshUlCqiMaps ();

  // Generate RBs map
  FfMacSchedSapUser::SchedUlConfigIndParameters ret;
  std::vector <bool> rbMap;
  uint16_t rbAllocatedNum = 0;
  std::set <uint16_t> rntiAllocated;
  std::vector <uint16_t> rbgAllocationMap;
  // update with RACH allocation map
  rbgAllocationMap = m_rachAllocationMap;
  //rbgAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  m_rachAllocationMap.clear ();
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);

  rbMap.resize (m_cschedCellConfig.m_ulBandwidth, false);
  // remove RACH allocation
  for (uint16_t i = 0; i < m_cschedCellConfig.m_ulBandwidth; i++)
    {
      if (rbgAllocationMap.at (i) != 0)
        {
          rbMap.at (i) = true;
          NS_LOG_DEBUG (this << " Allocated for RACH " << i);
        }
    }

  if (m_harqOn == true)
    {
      //   Process UL HARQ feedback
      for (uint16_t i = 0; i < params.m_ulInfoList.size (); i++)
        {
          if (params.m_ulInfoList.at (i).m_receptionStatus == UlInfoListElement_s::NotOk)
            {
              // retx correspondent block: retrieve the UL-DCI
              uint16_t rnti = params.m_ulInfoList.at (i).m_rnti;
              std::map <uint16_t, uint8_t>::iterator itProcId = m_ulHarqCurrentProcessId.find (rnti);
              if (itProcId == m_ulHarqCurrentProcessId.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                }
              uint8_t harqId = (uint8_t)((*itProcId).second - HARQ_PERIOD) % HARQ_PROC_NUM;
              NS_LOG_INFO (this << " UL-HARQ retx RNTI " << rnti << " harqId " << (uint16_t)harqId);
              std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itHarq = m_ulHarqProcessesDciBuffer.find (rnti);
              if (itHarq == m_ulHarqProcessesDciBuffer.end ())
                {
                  NS_LOG_ERROR ("No info find in UL-HARQ buffer for UE (might change eNB) " << rnti);
                }
              UlDciListElement_s dci = (*itHarq).second.at (harqId);
              std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (rnti);
              if (itStat == m_ulHarqProcessesStatus.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                }
              if ((*itStat).second.at (harqId) >= 3)
                {
                  NS_LOG_INFO ("Max number of retransmissions reached (UL)-> drop process");
                  continue;
                }
              bool free = true;
              for (int j = dci.m_rbStart; j < dci.m_rbStart + dci.m_rbLen; j++)
                {
                  if (rbMap.at (j) == true)
                    {
                      free = false;
                      NS_LOG_INFO (this << " BUSY " << j);
                    }
                }
              if (free)
                {
                  // retx on the same RBs
                  for (int j = dci.m_rbStart; j < dci.m_rbStart + dci.m_rbLen; j++)
                    {
                      rbMap.at (j) = true;
                      rbgAllocationMap.at (j) = dci.m_rnti;
                      NS_LOG_INFO ("\tRB " << j);
                      rbAllocatedNum++;
                    }
                  NS_LOG_INFO (this << " Send retx in the same RBGs " << (uint16_t)dci.m_rbStart << " to " << dci.m_rbStart + dci.m_rbLen << " RV " << (*itStat).second.at (harqId) + 1);
                }
              else
                {
                  NS_LOG_INFO ("Cannot allocate retx due to RACH allocations for UE " << rnti);
                  continue;
                }
              dci.m_ndi = 0;
              // Update HARQ buffers with new HarqId
              (*itStat).second.at ((*itProcId).second) = (*itStat).second.at (harqId) + 1;
              (*itStat).second.at (harqId) = 0;
              (*itHarq).second.at ((*itProcId).second) = dci;
              ret.m_dciList.push_back (dci);
              rntiAllocated.insert (dci.m_rnti);
            }
        }
    }

  std::map <uint16_t,uint32_t>::iterator it;
  int nflows = 0;

  for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
      // select UEs with queues not empty and not yet allocated for HARQ
      NS_LOG_INFO (this << " UE " << (*it).first << " queue " << (*it).second);
      if (((*it).second > 0)&&(itRnti == rntiAllocated.end ()))
        {
          nflows++;
        }
    }

  if (nflows == 0)
    {
      if (ret.m_dciList.size () > 0)
        {
          m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
          m_schedSapUser->SchedUlConfigInd (ret);
        }
      return;  // no flows to be scheduled
    }


  // Divide the remaining resources equally among the active users starting from the subsequent one served last scheduling trigger
  uint16_t rbPerFlow = (m_cschedCellConfig.m_ulBandwidth) / (nflows + rntiAllocated.size ());
  if (rbPerFlow < 3)
    {
      rbPerFlow = 3;  // at least 3 rbg per flow (till available resource) to ensure TxOpportunity >= 7 bytes
    }
  uint16_t rbAllocated = 0;

  if (m_nextRntiUl != 0)
    {
      for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
        {
          if ((*it).first == m_nextRntiUl)
            {
              break;
            }
        }
      if (it == m_ceBsrRxed.end ())
        {
          NS_LOG_ERROR (this << " no user found");
        }
    }
  else
    {
      it = m_ceBsrRxed.begin ();
      m_nextRntiUl = (*it).first;
    }
  NS_LOG_INFO (this << " NFlows " << nflows << " RB per Flow " << rbPerFlow);
  do
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
      if ((itRnti != rntiAllocated.end ())||((*it).second == 0))
        {
          // UE already allocated for UL-HARQ -> skip it
          it++;
          if (it == m_ceBsrRxed.end ())
            {
              // restart from the first
              it = m_ceBsrRxed.begin ();
            }
          continue;
        }
      if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
        {
          // limit to physical resources last resource assignment
          rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;
          // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
          if (rbPerFlow < 3)
            {
              // terminate allocation
              rbPerFlow = 0;      
            }
        }
      NS_LOG_INFO (this << " try to allocate " << (*it).first);
      UlDciListElement_s uldci;
      uldci.m_rnti = (*it).first;
      uldci.m_rbLen = rbPerFlow;
      bool allocated = false;
      NS_LOG_INFO (this << " RB Allocated " << rbAllocated << " rbPerFlow " << rbPerFlow << " flows " << nflows);
      while ((!allocated)&&((rbAllocated + rbPerFlow - m_cschedCellConfig.m_ulBandwidth) < 1) && (rbPerFlow != 0))
        {
          // check availability
          bool free = true;
          for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
            {
              if (rbMap.at (j) == true)
                {
                  free = false;
                  break;
                }
            }
          if (free)
            {
              uldci.m_rbStart = rbAllocated;

              for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
                {
                  rbMap.at (j) = true;
                  // store info on allocation for managing ul-cqi interpretation
                  rbgAllocationMap.at (j) = (*it).first;
                  NS_LOG_INFO ("\t " << j);
                }
              rbAllocated += rbPerFlow;
              allocated = true;
              break;
            }
          rbAllocated++;
          if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
            {
              // limit to physical resources last resource assignment
              rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;
              // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
              if (rbPerFlow < 3)
                {
                  // terminate allocation
                  rbPerFlow = 0;                 
                }
            }
        }
      if (!allocated)
        {
          // unable to allocate new resource: finish scheduling
          m_nextRntiUl = (*it).first;
          if (ret.m_dciList.size () > 0)
            {
              m_schedSapUser->SchedUlConfigInd (ret);
            }
          m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
          return;
        }
      std::map <uint16_t, std::vector <double> >::iterator itCqi = m_ueCqi.find ((*it).first);
      int cqi = 0;
      if (itCqi == m_ueCqi.end ())
        {
          // no cqi info about this UE
          uldci.m_mcs = 0; // MCS 0 -> UL-AMC TBD
          NS_LOG_INFO (this << " UE does not have ULCQI " << (*it).first );
        }
      else
        {
          // take the lowest CQI value (worst RB)
    	  NS_ABORT_MSG_IF ((*itCqi).second.size() == 0, "CQI of RNTI = " << (*it).first << " has expired");
          double minSinr = (*itCqi).second.at (uldci.m_rbStart);
          for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
            {
              if ((*itCqi).second.at (i) < minSinr)
                {
                  minSinr = (*itCqi).second.at (i);
                }
            }
          // translate SINR -> cqi: WILD ACK: same as DL
          double s = log2 ( 1 + (
                                 std::pow (10, minSinr / 10 )  /
                                 ( (-std::log (5.0 * 0.00005 )) / 1.5) ));


          cqi = m_amc->GetCqiFromSpectralEfficiency (s);
          if (cqi == 0)
            {
              it++;
              if (it == m_ceBsrRxed.end ())
                {
                  // restart from the first
                  it = m_ceBsrRxed.begin ();
                }
              NS_LOG_DEBUG (this << " UE discarded for CQI = 0, RNTI " << uldci.m_rnti);
              // remove UE from allocation map
              for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
                {
                  rbgAllocationMap.at (i) = 0;
                }
              continue; // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
            }
          uldci.m_mcs = m_amc->GetMcsFromCqi (cqi);
        }
      uldci.m_tbSize = (m_amc->GetUlTbSizeFromMcs (uldci.m_mcs, rbPerFlow) / 8); // MCS 0 -> UL-AMC TBD

      UpdateUlRlcBufferInfo (uldci.m_rnti, uldci.m_tbSize);
      uldci.m_ndi = 1;
      uldci.m_cceIndex = 0;
      uldci.m_aggrLevel = 1;
      uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
      uldci.m_hopping = false;
      uldci.m_n2Dmrs = 0;
      uldci.m_tpc = 0; // no power control
      uldci.m_cqiRequest = false; // only period CQI at this stage
      uldci.m_ulIndex = 0; // TDD parameter
      uldci.m_dai = 1; // TDD parameter
      uldci.m_freqHopping = 0;
      uldci.m_pdcchPowerOffset = 0; // not used
      ret.m_dciList.push_back (uldci);
      // store DCI for HARQ_PERIOD
      uint8_t harqId = 0;
      if (m_harqOn == true)
        {
          std::map <uint16_t, uint8_t>::iterator itProcId;
          itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
          if (itProcId == m_ulHarqCurrentProcessId.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
            }
          harqId = (*itProcId).second;
          std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
          if (itDci == m_ulHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
            }
          (*itDci).second.at (harqId) = uldci;
          // Update HARQ process status (RV 0)
          std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (uldci.m_rnti);
          if (itStat == m_ulHarqProcessesStatus.end ())
            {
              NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << uldci.m_rnti);
            }
          (*itStat).second.at (harqId) = 0;
        }
        
      NS_LOG_INFO (this << " UL Allocation - UE " << (*it).first << " startPRB " << (uint32_t)uldci.m_rbStart << " nPRB " << (uint32_t)uldci.m_rbLen << " CQI " << cqi << " MCS " << (uint32_t)uldci.m_mcs << " TBsize " << uldci.m_tbSize << " harqId " << (uint16_t)harqId);

      it++;
      if (it == m_ceBsrRxed.end ())
        {
          // restart from the first
          it = m_ceBsrRxed.begin ();
        }
      if ((rbAllocated == m_cschedCellConfig.m_ulBandwidth) || (rbPerFlow == 0))
        {
          // Stop allocation: no more PRBs
          m_nextRntiUl = (*it).first;
          break;
        }
    }
  while (((*it).first != m_nextRntiUl)&&(rbPerFlow!=0));

  m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));

  m_schedSapUser->SchedUlConfigInd (ret);
  return;
  }
  else if(m_flag == 2)
  {

  NS_LOG_FUNCTION (this << " UL - Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf) << " size " << params.m_ulInfoList.size ());

  RefreshUlCqiMaps ();

  // Generate RBs map
  FfMacSchedSapUser::SchedUlConfigIndParameters ret;
  std::vector <bool> rbMap;
  uint16_t rbAllocatedNum = 0;
  std::set <uint16_t> rntiAllocated;
  std::vector <uint16_t> rbgAllocationMap;
  // update with RACH allocation map
  rbgAllocationMap = m_rachAllocationMap;
  //rbgAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  m_rachAllocationMap.clear ();
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);

  rbMap.resize (m_cschedCellConfig.m_ulBandwidth, false);
  // remove RACH allocation
  for (uint16_t i = 0; i < m_cschedCellConfig.m_ulBandwidth; i++)
    {
      if (rbgAllocationMap.at (i) != 0)
        {
          rbMap.at (i) = true;
          NS_LOG_DEBUG (this << " Allocated for RACH " << i);
        }
    }


  if (m_harqOn == true)
    {
      //   Process UL HARQ feedback
      for (uint16_t i = 0; i < params.m_ulInfoList.size (); i++)
        {
          if (params.m_ulInfoList.at (i).m_receptionStatus == UlInfoListElement_s::NotOk)
            {
              // retx correspondent block: retrieve the UL-DCI
              uint16_t rnti = params.m_ulInfoList.at (i).m_rnti;
              std::map <uint16_t, uint8_t>::iterator itProcId = m_ulHarqCurrentProcessId.find (rnti);
              if (itProcId == m_ulHarqCurrentProcessId.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                }
              uint8_t harqId = (uint8_t)((*itProcId).second - HARQ_PERIOD) % HARQ_PROC_NUM;
              NS_LOG_INFO (this << " UL-HARQ retx RNTI " << rnti << " harqId " << (uint16_t)harqId << " i " << i << " size "  << params.m_ulInfoList.size ());
              std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itHarq = m_ulHarqProcessesDciBuffer.find (rnti);
              if (itHarq == m_ulHarqProcessesDciBuffer.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                  continue;
                }
              UlDciListElement_s dci = (*itHarq).second.at (harqId);
              std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (rnti);
              if (itStat == m_ulHarqProcessesStatus.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                }
              if ((*itStat).second.at (harqId) >= 3)
                {
                  NS_LOG_INFO ("Max number of retransmissions reached (UL)-> drop process");
                  continue;
                }
              bool free = true;
              for (int j = dci.m_rbStart; j < dci.m_rbStart + dci.m_rbLen; j++)
                {
                  if (rbMap.at (j) == true)
                    {
                      free = false;
                      NS_LOG_INFO (this << " BUSY " << j);
                    }
                }
              if (free)
                {
                  // retx on the same RBs
                  for (int j = dci.m_rbStart; j < dci.m_rbStart + dci.m_rbLen; j++)
                    {
                      rbMap.at (j) = true;
                      rbgAllocationMap.at (j) = dci.m_rnti;
                      NS_LOG_INFO ("\tRB " << j);
                      rbAllocatedNum++;
                    }
                  NS_LOG_INFO (this << " Send retx in the same RBs " << (uint16_t)dci.m_rbStart << " to " << dci.m_rbStart + dci.m_rbLen << " RV " << (*itStat).second.at (harqId) + 1);
                }
              else
                {
                  NS_LOG_INFO ("Cannot allocate retx due to RACH allocations for UE " << rnti);
                  continue;
                }
              dci.m_ndi = 0;
              // Update HARQ buffers with new HarqId
              (*itStat).second.at ((*itProcId).second) = (*itStat).second.at (harqId) + 1;
              (*itStat).second.at (harqId) = 0;
              (*itHarq).second.at ((*itProcId).second) = dci;
              ret.m_dciList.push_back (dci);
              rntiAllocated.insert (dci.m_rnti);
            }
          else
            {
              NS_LOG_INFO (this << " HARQ-ACK feedback from RNTI " << params.m_ulInfoList.at (i).m_rnti);
            }
        }
    }

  std::map <uint16_t,uint32_t>::iterator it;
  int nflows = 0;

  for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
      // select UEs with queues not empty and not yet allocated for HARQ
      if (((*it).second > 0)&&(itRnti == rntiAllocated.end ()))
        {
          nflows++;
        }
    }

  if (nflows == 0)
    {
      if (ret.m_dciList.size () > 0)
        {
          m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
          m_schedSapUser->SchedUlConfigInd (ret);
        }

      return;  // no flows to be scheduled
    }


  // Divide the remaining resources equally among the active users starting from the subsequent one served last scheduling trigger
  uint16_t rbPerFlow = (m_cschedCellConfig.m_ulBandwidth) / (nflows + rntiAllocated.size ());
  if (rbPerFlow < 3)
    {
      rbPerFlow = 3;  // at least 3 rbg per flow (till available resource) to ensure TxOpportunity >= 7 bytes
    }
  int rbAllocated = 0;

  std::map <uint16_t, RrsFlowPerf_t>::iterator itStats;
  if (m_nextRntiUl != 0)
    {
      for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
        {
          if ((*it).first == m_nextRntiUl)
            {
              break;
            }
        }
      if (it == m_ceBsrRxed.end ())
        {
          NS_LOG_ERROR (this << " no user found");
        }
    }
  else
    {
      it = m_ceBsrRxed.begin ();
      m_nextRntiUl = (*it).first;
    }
  do
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
      if ((itRnti != rntiAllocated.end ())||((*it).second == 0))
        {
          // UE already allocated for UL-HARQ -> skip it
          NS_LOG_DEBUG (this << " UE already allocated in HARQ -> discared, RNTI " << (*it).first);
          it++;
          if (it == m_ceBsrRxed.end ())
            {
              // restart from the first
              it = m_ceBsrRxed.begin ();
            }
          continue;
        }
      if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
        {
          // limit to physical resources last resource assignment
          rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;
          // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
          if (rbPerFlow < 3)
            {
              // terminate allocation
              rbPerFlow = 0;
            }
        }

      UlDciListElement_s uldci;
      uldci.m_rnti = (*it).first;
      uldci.m_rbLen = rbPerFlow;
      bool allocated = false;
      NS_LOG_INFO (this << " RB Allocated " << rbAllocated << " rbPerFlow " << rbPerFlow << " flows " << nflows);
      while ((!allocated)&&((rbAllocated + rbPerFlow - m_cschedCellConfig.m_ulBandwidth) < 1) && (rbPerFlow != 0))
        {
          // check availability
          bool free = true;
          for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
            {
              if (rbMap.at (j) == true)
                {
                  free = false;
                  break;
                }
            }
          if (free)
            {
              uldci.m_rbStart = rbAllocated;

              for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
                {
                  rbMap.at (j) = true;
                  // store info on allocation for managing ul-cqi interpretation
                  rbgAllocationMap.at (j) = (*it).first;
                }
              rbAllocated += rbPerFlow;
              allocated = true;
              break;
            }
          rbAllocated++;
          if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
            {
              // limit to physical resources last resource assignment
              rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;
              // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
              if (rbPerFlow < 3)
                {
                  // terminate allocation
                  rbPerFlow = 0;
                }
            }
        }
      if (!allocated)
        {
          // unable to allocate new resource: finish scheduling
          m_nextRntiUl = (*it).first;
          if (ret.m_dciList.size () > 0)
            {
              m_schedSapUser->SchedUlConfigInd (ret);
            }
          m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
          return;
        }



      std::map <uint16_t, std::vector <double> >::iterator itCqi = m_ueCqi.find ((*it).first);
      int cqi = 0;
      if (itCqi == m_ueCqi.end ())
        {
          // no cqi info about this UE
          uldci.m_mcs = 0; // MCS 0 -> UL-AMC TBD
        }
      else
        {
          // take the lowest CQI value (worst RB)
    	  NS_ABORT_MSG_IF ((*itCqi).second.size() == 0, "CQI of RNTI = " << (*it).first << " has expired");
          double minSinr = (*itCqi).second.at (uldci.m_rbStart);
          if (minSinr == NO_SINR)
            {
              minSinr = EstimateUlSinr ((*it).first, uldci.m_rbStart);
            }
          for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
            {
              double sinr = (*itCqi).second.at (i);
              if (sinr == NO_SINR)
                {
                  sinr = EstimateUlSinr ((*it).first, i);
                }
              if (sinr < minSinr)
                {
                  minSinr = sinr;
                }
            }

          // translate SINR -> cqi: WILD ACK: same as DL
          double s = log2 ( 1 + (
                              std::pow (10, minSinr / 10 )  /
                              ( (-std::log (5.0 * 0.00005 )) / 1.5) ));
          cqi = m_amc->GetCqiFromSpectralEfficiency (s);
          if (cqi == 0)
            {
              it++;
              if (it == m_ceBsrRxed.end ())
                {
                  // restart from the first
                  it = m_ceBsrRxed.begin ();
                }
              NS_LOG_DEBUG (this << " UE discarded for CQI = 0, RNTI " << uldci.m_rnti);
              // remove UE from allocation map
              for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
                {
                  rbgAllocationMap.at (i) = 0;
                }
              continue; // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
            }
          uldci.m_mcs = m_amc->GetMcsFromCqi (cqi);
        }

      uldci.m_tbSize = (m_amc->GetUlTbSizeFromMcs (uldci.m_mcs, rbPerFlow) / 8);
      UpdateUlRlcBufferInfo (uldci.m_rnti, uldci.m_tbSize);
      uldci.m_ndi = 1;
      uldci.m_cceIndex = 0;
      uldci.m_aggrLevel = 1;
      uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
      uldci.m_hopping = false;
      uldci.m_n2Dmrs = 0;
      uldci.m_tpc = 0; // no power control
      uldci.m_cqiRequest = false; // only period CQI at this stage
      uldci.m_ulIndex = 0; // TDD parameter
      uldci.m_dai = 1; // TDD parameter
      uldci.m_freqHopping = 0;
      uldci.m_pdcchPowerOffset = 0; // not used
      ret.m_dciList.push_back (uldci);
      // store DCI for HARQ_PERIOD
      uint8_t harqId = 0;
      if (m_harqOn == true)
        {
          std::map <uint16_t, uint8_t>::iterator itProcId;
          itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
          if (itProcId == m_ulHarqCurrentProcessId.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
            }
          harqId = (*itProcId).second;
          std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
          if (itDci == m_ulHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
            }
          (*itDci).second.at (harqId) = uldci;
          // Update HARQ process status (RV 0)
          std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (uldci.m_rnti);
          if (itStat == m_ulHarqProcessesStatus.end ())
            {
              NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << uldci.m_rnti);
            }
          (*itStat).second.at (harqId) = 0;
        }

      NS_LOG_INFO (this << " UE Allocation RNTI " << (*it).first << " startPRB " << (uint32_t)uldci.m_rbStart << " nPRB " << (uint32_t)uldci.m_rbLen << " CQI " << cqi << " MCS " << (uint32_t)uldci.m_mcs << " TBsize " << uldci.m_tbSize << " RbAlloc " << rbAllocated << " harqId " << (uint16_t)harqId);

      // update TTI  UE stats
      itStats = m_flowStatsUl.find ((*it).first);
      if (itStats != m_flowStatsUl.end ())
        {
          (*itStats).second.lastTtiBytesTransmitted =  uldci.m_tbSize;
        }
      else
        {
          NS_LOG_DEBUG (this << " No Stats for this allocated UE");
        }


      it++;
      if (it == m_ceBsrRxed.end ())
        {
          // restart from the first
          it = m_ceBsrRxed.begin ();
        }
      if ((rbAllocated == m_cschedCellConfig.m_ulBandwidth) || (rbPerFlow == 0))
        {
          // Stop allocation: no more PRBs
          m_nextRntiUl = (*it).first;
          break;
        }
    }
  while (((*it).first != m_nextRntiUl)&&(rbPerFlow!=0));


  // Update global UE stats
  // update UEs stats
  for (itStats = m_flowStatsUl.begin (); itStats != m_flowStatsUl.end (); itStats++)
    {
      (*itStats).second.totalBytesTransmitted += (*itStats).second.lastTtiBytesTransmitted;
      // update average throughput (see eq. 12.3 of Sec 12.3.1.2 of LTE – The UMTS Long Term Evolution, Ed Wiley)
      (*itStats).second.lastAveragedThroughput = ((1.0 - (1.0 / m_timeWindow)) * (*itStats).second.lastAveragedThroughput) + ((1.0 / m_timeWindow) * (double)((*itStats).second.lastTtiBytesTransmitted / 0.001));
      NS_LOG_INFO (this << " UE total bytes " << (*itStats).second.totalBytesTransmitted);
      NS_LOG_INFO (this << " UE average throughput " << (*itStats).second.lastAveragedThroughput);
      (*itStats).second.lastTtiBytesTransmitted = 0;
    }
  m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
  m_schedSapUser->SchedUlConfigInd (ret);

  return;
  }
  else if(m_flag == 3)
  {

  NS_LOG_FUNCTION (this << " UL - Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf) << " size " << params.m_ulInfoList.size ());

  RefreshUlCqiMaps ();
  m_ffrSapProvider->ReportUlCqiInfo (m_ueCqi);

  // Generate RBs map
  FfMacSchedSapUser::SchedUlConfigIndParameters ret;
  std::vector <bool> rbMap;
  uint16_t rbAllocatedNum = 0;
  std::set <uint16_t> rntiAllocated;
  std::vector <uint16_t> rbgAllocationMap;
  // update with RACH allocation map
  rbgAllocationMap = m_rachAllocationMap;
  //rbgAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  m_rachAllocationMap.clear ();
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);

  rbMap.resize (m_cschedCellConfig.m_ulBandwidth, false);

  rbMap = m_ffrSapProvider->GetAvailableUlRbg ();

  for (std::vector<bool>::iterator it = rbMap.begin (); it != rbMap.end (); it++)
    {
      if ((*it) == true )
        {
          rbAllocatedNum++;
        }
    }

  uint8_t minContinuousUlBandwidth = m_ffrSapProvider->GetMinContinuousUlBandwidth ();
  uint8_t ffrUlBandwidth = m_cschedCellConfig.m_ulBandwidth - rbAllocatedNum;


  // remove RACH allocation
  for (uint16_t i = 0; i < m_cschedCellConfig.m_ulBandwidth; i++)
    {
      if (rbgAllocationMap.at (i) != 0)
        {
          rbMap.at (i) = true;
          NS_LOG_DEBUG (this << " Allocated for RACH " << i);
        }
    }


  if (m_harqOn == true)
    {
      //   Process UL HARQ feedback
      for (uint16_t i = 0; i < params.m_ulInfoList.size (); i++)
        {        
          if (params.m_ulInfoList.at (i).m_receptionStatus == UlInfoListElement_s::NotOk)
            {
              // retx correspondent block: retrieve the UL-DCI
              uint16_t rnti = params.m_ulInfoList.at (i).m_rnti;
              std::map <uint16_t, uint8_t>::iterator itProcId = m_ulHarqCurrentProcessId.find (rnti);
              if (itProcId == m_ulHarqCurrentProcessId.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                }
              uint8_t harqId = (uint8_t)((*itProcId).second - HARQ_PERIOD) % HARQ_PROC_NUM;
              NS_LOG_INFO (this << " UL-HARQ retx RNTI " << rnti << " harqId " << (uint16_t)harqId << " i " << i << " size "  << params.m_ulInfoList.size ());
              std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itHarq = m_ulHarqProcessesDciBuffer.find (rnti);
              if (itHarq == m_ulHarqProcessesDciBuffer.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                  continue;
                }
              UlDciListElement_s dci = (*itHarq).second.at (harqId);
              std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (rnti);
              if (itStat == m_ulHarqProcessesStatus.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                }
              if ((*itStat).second.at (harqId) >= 3)
                {
                  NS_LOG_INFO ("Max number of retransmissions reached (UL)-> drop process");
                  continue;
                }
              bool free = true;
              for (int j = dci.m_rbStart; j < dci.m_rbStart + dci.m_rbLen; j++)
                {
                  if (rbMap.at (j) == true)
                    {
                      free = false;
                      NS_LOG_INFO (this << " BUSY " << j);
                    }
                }
              if (free)
                {
                  // retx on the same RBs
                  for (int j = dci.m_rbStart; j < dci.m_rbStart + dci.m_rbLen; j++)
                    {
                      rbMap.at (j) = true;
                      rbgAllocationMap.at (j) = dci.m_rnti;
                      NS_LOG_INFO ("\tRB " << j);
                      rbAllocatedNum++;
                    }
                  NS_LOG_INFO (this << " Send retx in the same RBs " << (uint16_t)dci.m_rbStart << " to " << dci.m_rbStart + dci.m_rbLen << " RV " << (*itStat).second.at (harqId) + 1);
                }
              else
                {
                  NS_LOG_INFO ("Cannot allocate retx due to RACH allocations for UE " << rnti);
                  continue;
                }
              dci.m_ndi = 0;
              // Update HARQ buffers with new HarqId
              (*itStat).second.at ((*itProcId).second) = (*itStat).second.at (harqId) + 1;
              (*itStat).second.at (harqId) = 0;
              (*itHarq).second.at ((*itProcId).second) = dci;
              ret.m_dciList.push_back (dci);
              rntiAllocated.insert (dci.m_rnti);
            }
            else
            {
              NS_LOG_INFO (this << " HARQ-ACK feedback from RNTI " << params.m_ulInfoList.at (i).m_rnti);
            }
        }
    }

  std::map <uint16_t,uint32_t>::iterator it;
  int nflows = 0;

  for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
      // select UEs with queues not empty and not yet allocated for HARQ
      if (((*it).second > 0)&&(itRnti == rntiAllocated.end ()))
        {
          nflows++;
        }
    }

  if (nflows == 0)
    {
      if (ret.m_dciList.size () > 0)
        {
          m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
          m_schedSapUser->SchedUlConfigInd (ret);
        }
        
      return;  // no flows to be scheduled
    }


  // Divide the remaining resources equally among the active users starting from the subsequent one served last scheduling trigger
  uint16_t tempRbPerFlow = (ffrUlBandwidth) / (nflows + rntiAllocated.size ());
  uint16_t rbPerFlow = (minContinuousUlBandwidth < tempRbPerFlow) ? minContinuousUlBandwidth : tempRbPerFlow;

  if (rbPerFlow < 3)
    {
      rbPerFlow = 3;  // at least 3 rbg per flow (till available resource) to ensure TxOpportunity >= 7 bytes
    }
  int rbAllocated = 0;

  std::map <uint16_t, RrsFlowPerf_t>::iterator itStats;
  if (m_nextRntiUl != 0)
    {
      for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
        {
          if ((*it).first == m_nextRntiUl)
            {
              break;
            }
        }
      if (it == m_ceBsrRxed.end ())
        {
          NS_LOG_ERROR (this << " no user found");
        }
    }
  else
    {
      it = m_ceBsrRxed.begin ();
      m_nextRntiUl = (*it).first;
    }
  do
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
      if ((itRnti != rntiAllocated.end ())||((*it).second == 0))
        {
          // UE already allocated for UL-HARQ -> skip it
          NS_LOG_DEBUG (this << " UE already allocated in HARQ -> discared, RNTI " << (*it).first);
          it++;
          if (it == m_ceBsrRxed.end ())
            {
              // restart from the first
              it = m_ceBsrRxed.begin ();
            }
          continue;
        }
      if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
        {
          // limit to physical resources last resource assignment
          rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;
          // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
          if (rbPerFlow < 3)
            {
              // terminate allocation
              rbPerFlow = 0;      
            }
        }

      rbAllocated = 0;
      UlDciListElement_s uldci;
      uldci.m_rnti = (*it).first;
      uldci.m_rbLen = rbPerFlow;
      bool allocated = false;
      NS_LOG_INFO (this << " RB Allocated " << rbAllocated << " rbPerFlow " << rbPerFlow << " flows " << nflows);
      while ((!allocated)&&((rbAllocated + rbPerFlow - m_cschedCellConfig.m_ulBandwidth) < 1) && (rbPerFlow != 0))
        {
          // check availability
          bool free = true;
          for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
            {
              if (rbMap.at (j) == true)
                {
                  free = false;
                  break;
                }
              if ((m_ffrSapProvider->IsUlRbgAvailableForUe (j, (*it).first)) == false)
                {
                  free = false;
                  break;
                }
            }
          if (free)
            {
        	  NS_LOG_INFO (this << "RNTI: "<< (*it).first<< " RB Allocated " << rbAllocated << " rbPerFlow " << rbPerFlow << " flows " << nflows);
              uldci.m_rbStart = rbAllocated;

              for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
                {
                  rbMap.at (j) = true;
                  // store info on allocation for managing ul-cqi interpretation
                  rbgAllocationMap.at (j) = (*it).first;
                }
              rbAllocated += rbPerFlow;
              allocated = true;
              break;
            }
          rbAllocated++;
          if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
            {
              // limit to physical resources last resource assignment
              rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;
              // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
              if (rbPerFlow < 3)
                {
                  // terminate allocation
                  rbPerFlow = 0;                 
                }
            }
        }
      if (!allocated)
        {
          // unable to allocate new resource: finish scheduling
  //          m_nextRntiUl = (*it).first;
  //          if (ret.m_dciList.size () > 0)
  //            {
  //              m_schedSapUser->SchedUlConfigInd (ret);
  //            }
  //          m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
  //          return;
          break;
        }



      std::map <uint16_t, std::vector <double> >::iterator itCqi = m_ueCqi.find ((*it).first);
      int cqi = 0;
      if (itCqi == m_ueCqi.end ())
        {
          // no cqi info about this UE
          uldci.m_mcs = 0; // MCS 0 -> UL-AMC TBD
        }
      else
        {
          // take the lowest CQI value (worst RB)
    	  NS_ABORT_MSG_IF ((*itCqi).second.size() == 0, "CQI of RNTI = " << (*it).first << " has expired");
          double minSinr = (*itCqi).second.at (uldci.m_rbStart);
          if (minSinr == NO_SINR)
            {
              minSinr = EstimateUlSinr ((*it).first, uldci.m_rbStart);
            }
          for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
            {
              double sinr = (*itCqi).second.at (i);
              if (sinr == NO_SINR)
                {
                  sinr = EstimateUlSinr ((*it).first, i);
                }
              if (sinr < minSinr)
                {
                  minSinr = sinr;
                }
            }

          // translate SINR -> cqi: WILD ACK: same as DL
          double s = log2 ( 1 + (
                                 std::pow (10, minSinr / 10 )  /
                                 ( (-std::log (5.0 * 0.00005 )) / 1.5) ));
          cqi = m_amc->GetCqiFromSpectralEfficiency (s);
          if (cqi == 0)
            {
              it++;
              if (it == m_ceBsrRxed.end ())
                {
                  // restart from the first
                  it = m_ceBsrRxed.begin ();
                }
              NS_LOG_DEBUG (this << " UE discarded for CQI = 0, RNTI " << uldci.m_rnti);
              // remove UE from allocation map
              for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
                {
                  rbgAllocationMap.at (i) = 0;
                }
              continue; // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
            }
          uldci.m_mcs = m_amc->GetMcsFromCqi (cqi);
        }

      uldci.m_tbSize = (m_amc->GetUlTbSizeFromMcs (uldci.m_mcs, rbPerFlow) / 8);
      UpdateUlRlcBufferInfo (uldci.m_rnti, uldci.m_tbSize);
      uldci.m_ndi = 1;
      uldci.m_cceIndex = 0;
      uldci.m_aggrLevel = 1;
      uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
      uldci.m_hopping = false;
      uldci.m_n2Dmrs = 0;
      uldci.m_tpc = 0; // no power control
      uldci.m_cqiRequest = false; // only period CQI at this stage
      uldci.m_ulIndex = 0; // TDD parameter
      uldci.m_dai = 1; // TDD parameter
      uldci.m_freqHopping = 0;
      uldci.m_pdcchPowerOffset = 0; // not used
      ret.m_dciList.push_back (uldci);
      // store DCI for HARQ_PERIOD
      uint8_t harqId = 0;
      if (m_harqOn == true)
        {
          std::map <uint16_t, uint8_t>::iterator itProcId;
          itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
          if (itProcId == m_ulHarqCurrentProcessId.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
            }
          harqId = (*itProcId).second;
          std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
          if (itDci == m_ulHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
            }
          (*itDci).second.at (harqId) = uldci;
          // Update HARQ process status (RV 0)
          std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (uldci.m_rnti);
          if (itStat == m_ulHarqProcessesStatus.end ())
            {
              NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << uldci.m_rnti);
            }
          (*itStat).second.at (harqId) = 0;
        }

      NS_LOG_INFO (this << " UE Allocation RNTI " << (*it).first << " startPRB " << (uint32_t)uldci.m_rbStart << " nPRB " << (uint32_t)uldci.m_rbLen << " CQI " << cqi << " MCS " << (uint32_t)uldci.m_mcs << " TBsize " << uldci.m_tbSize << " RbAlloc " << rbAllocated << " harqId " << (uint16_t)harqId);

      it++;
      if (it == m_ceBsrRxed.end ())
        {
          // restart from the first
          it = m_ceBsrRxed.begin ();
        }
      if ((rbAllocated == m_cschedCellConfig.m_ulBandwidth) || (rbPerFlow == 0))
        {
          // Stop allocation: no more PRBs
          m_nextRntiUl = (*it).first;
          break;
        }
    }
  while (((*it).first != m_nextRntiUl)&&(rbPerFlow!=0));

  m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
  m_schedSapUser->SchedUlConfigInd (ret);

  return;
  }
  else
  {
    // std::cout << "执行1" << std::endl;
    NS_LOG_FUNCTION (this << " UL - Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf) << " size " << params.m_ulInfoList.size ());

  RefreshUlCqiMaps ();
  m_ffrSapProvider->ReportUlCqiInfo (m_ueCqi);

  // Generate RBs map
  FfMacSchedSapUser::SchedUlConfigIndParameters ret;
  std::vector <bool> rbMap;
  uint16_t rbAllocatedNum = 0;
  std::set <uint16_t> rntiAllocated;
  std::vector <uint16_t> rbgAllocationMap;
  // update with RACH allocation map
  rbgAllocationMap = m_rachAllocationMap;
  //rbgAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  m_rachAllocationMap.clear ();
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);

  rbMap.resize (m_cschedCellConfig.m_ulBandwidth, false);

  rbMap = m_ffrSapProvider->GetAvailableUlRbg ();

  for (std::vector<bool>::iterator it = rbMap.begin (); it != rbMap.end (); it++)
    {
      if ((*it) == true )
        {
          rbAllocatedNum++;
        }
    }

  uint8_t minContinuousUlBandwidth = m_ffrSapProvider->GetMinContinuousUlBandwidth ();
  uint8_t ffrUlBandwidth = m_cschedCellConfig.m_ulBandwidth - rbAllocatedNum;

  // remove RACH allocation
  for (uint16_t i = 0; i < m_cschedCellConfig.m_ulBandwidth; i++)
    {
      if (rbgAllocationMap.at (i) != 0)
        {
          rbMap.at (i) = true;
          NS_LOG_DEBUG (this << " Allocated for RACH " << i);
        }
    }


  if (m_harqOn == true)
    {
      //   Process UL HARQ feedback
      for (uint16_t i = 0; i < params.m_ulInfoList.size (); i++)
        {
          if (params.m_ulInfoList.at (i).m_receptionStatus == UlInfoListElement_s::NotOk)
            {
              // retx correspondent block: retrieve the UL-DCI
              uint16_t rnti = params.m_ulInfoList.at (i).m_rnti;
              std::map <uint16_t, uint8_t>::iterator itProcId = m_ulHarqCurrentProcessId.find (rnti);
              if (itProcId == m_ulHarqCurrentProcessId.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                }
              uint8_t harqId = (uint8_t)((*itProcId).second - HARQ_PERIOD) % HARQ_PROC_NUM;
              NS_LOG_INFO (this << " UL-HARQ retx RNTI " << rnti << " harqId " << (uint16_t)harqId << " i " << i << " size "  << params.m_ulInfoList.size ());
              std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itHarq = m_ulHarqProcessesDciBuffer.find (rnti);
              if (itHarq == m_ulHarqProcessesDciBuffer.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                  continue;
                }
              UlDciListElement_s dci = (*itHarq).second.at (harqId);
              std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (rnti);
              if (itStat == m_ulHarqProcessesStatus.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                }
              if ((*itStat).second.at (harqId) >= 3)
                {
                  NS_LOG_INFO ("Max number of retransmissions reached (UL)-> drop process");
                  continue;
                }
              bool free = true;

              for (int j = dci.m_rbStart; j < dci.m_rbStart + dci.m_rbLen; j++)
                {
                  if (rbMap.at (j) == true)
                    {
                      free = false;
                      NS_LOG_INFO (this << " BUSY " << j);
                    }
                }
              if (free)
                {
                  // retx on the same RBs
                  for (int j = dci.m_rbStart; j < dci.m_rbStart + dci.m_rbLen; j++)
                    {
                      rbMap.at (j) = true;
                      rbgAllocationMap.at (j) = dci.m_rnti;
                      NS_LOG_INFO ("\tRB " << j);
                      rbAllocatedNum++;
                    }
                  NS_LOG_INFO (this << " Send retx in the same RBs " << (uint16_t)dci.m_rbStart << " to " << dci.m_rbStart + dci.m_rbLen << " RV " << (*itStat).second.at (harqId) + 1);
                }
              else
                {
                  NS_LOG_INFO ("Cannot allocate retx due to RACH allocations for UE " << rnti);
                  continue;
                }
              dci.m_ndi = 0;
              // Update HARQ buffers with new HarqId
              (*itStat).second.at ((*itProcId).second) = (*itStat).second.at (harqId) + 1;
              (*itStat).second.at (harqId) = 0;
              (*itHarq).second.at ((*itProcId).second) = dci;
              ret.m_dciList.push_back (dci);
              rntiAllocated.insert (dci.m_rnti);
            }
          else
            {
              NS_LOG_INFO (this << " HARQ-ACK feedback from RNTI " << params.m_ulInfoList.at (i).m_rnti);
            }
        }
    }

  std::map <uint16_t,uint32_t>::iterator it;
  int nflows = 0;

  for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
      // select UEs with queues not empty and not yet allocated for HARQ
      if (((*it).second > 0)&&(itRnti == rntiAllocated.end ()))
        {
          nflows++;
        }
    }

  if (nflows == 0)
    {
      if (ret.m_dciList.size () > 0)
        {
          m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
          m_schedSapUser->SchedUlConfigInd (ret);
        }

      return;  // no flows to be scheduled
    }


  // Divide the remaining resources equally among the active users starting from the subsequent one served last scheduling trigger
  uint16_t tempRbPerFlow = (ffrUlBandwidth) / (nflows + rntiAllocated.size ());
  uint16_t rbPerFlow = (minContinuousUlBandwidth < tempRbPerFlow) ? minContinuousUlBandwidth : tempRbPerFlow;

  if (rbPerFlow < 3)
    {
      rbPerFlow = 3;  // at least 3 rbg per flow (till available resource) to ensure TxOpportunity >= 7 bytes
    }
  int rbAllocated = 0;

  std::map <uint16_t, RrsFlowPerf_t>::iterator itStats;
  if (m_nextRntiUl != 0)
    {
      for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
        {
          if ((*it).first == m_nextRntiUl)
            {
              break;
            }
        }
      if (it == m_ceBsrRxed.end ())
        {
          NS_LOG_ERROR (this << " no user found");
        }
    }
  else
    {
      it = m_ceBsrRxed.begin ();
      m_nextRntiUl = (*it).first;
    }
  do
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
      if ((itRnti != rntiAllocated.end ())||((*it).second == 0))
        {
          // UE already allocated for UL-HARQ -> skip it
          NS_LOG_DEBUG (this << " UE already allocated in HARQ -> discared, RNTI " << (*it).first);
          it++;
          if (it == m_ceBsrRxed.end ())
            {
              // restart from the first
              it = m_ceBsrRxed.begin ();
            }
          continue;
        }
      if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
        {
          // limit to physical resources last resource assignment
          rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;
          // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
          if (rbPerFlow < 3)
            {
              // terminate allocation
              rbPerFlow = 0;
            }
        }

      rbAllocated = 0;
      UlDciListElement_s uldci;
      uldci.m_rnti = (*it).first;
      uldci.m_rbLen = rbPerFlow;
      bool allocated = false;
      NS_LOG_INFO (this << " RB Allocated " << rbAllocated << " rbPerFlow " << rbPerFlow << " flows " << nflows);
      while ((!allocated)&&((rbAllocated + rbPerFlow - m_cschedCellConfig.m_ulBandwidth) < 1) && (rbPerFlow != 0))
        {
          // check availability
          bool free = true;
          for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
            {
              if (rbMap.at (j) == true)
                {
                  free = false;
                  break;
                }
              if ((m_ffrSapProvider->IsUlRbgAvailableForUe (j, (*it).first)) == false)
                {
                  free = false;
                  break;
                }
            }
          if (free)
            {
        	  NS_LOG_INFO (this << "RNTI: "<< (*it).first<< " RB Allocated " << rbAllocated << " rbPerFlow " << rbPerFlow << " flows " << nflows);
              uldci.m_rbStart = rbAllocated;

              for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
                {
                  rbMap.at (j) = true;
                  // store info on allocation for managing ul-cqi interpretation
                  rbgAllocationMap.at (j) = (*it).first;
                }
              rbAllocated += rbPerFlow;
              allocated = true;
              break;
            }
          rbAllocated++;
          if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
            {
              // limit to physical resources last resource assignment
              rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;
              // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
              if (rbPerFlow < 3)
                {
                  // terminate allocation
                  rbPerFlow = 0;
                }
            }
        }
      if (!allocated)
        {
          // unable to allocate new resource: finish scheduling
  //          m_nextRntiUl = (*it).first;
  //          if (ret.m_dciList.size () > 0)
  //            {
  //              m_schedSapUser->SchedUlConfigInd (ret);
  //            }
  //          m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
  //          return;
          break;
        }



      std::map <uint16_t, std::vector <double> >::iterator itCqi = m_ueCqi.find ((*it).first);
      int cqi = 0;
      if (itCqi == m_ueCqi.end ())
        {
          // no cqi info about this UE
          uldci.m_mcs = 0; // MCS 0 -> UL-AMC TBD
        }
      else
        {
          // take the lowest CQI value (worst RB)
    	  NS_ABORT_MSG_IF ((*itCqi).second.size() == 0, "CQI of RNTI = " << (*it).first << " has expired");
          double minSinr = (*itCqi).second.at (uldci.m_rbStart);
          if (minSinr == NO_SINR)
            {
              minSinr = EstimateUlSinr ((*it).first, uldci.m_rbStart);
            }
          for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
            {
              double sinr = (*itCqi).second.at (i);
              if (sinr == NO_SINR)
                {
                  sinr = EstimateUlSinr ((*it).first, i);
                }
              if (sinr < minSinr)
                {
                  minSinr = sinr;
                }
            }

          // translate SINR -> cqi: WILD ACK: same as DL
          double s = log2 ( 1 + (
                              std::pow (10, minSinr / 10 )  /
                              ( (-std::log (5.0 * 0.00005 )) / 1.5) ));
          cqi = m_amc->GetCqiFromSpectralEfficiency (s);
          if (cqi == 0)
            {
              it++;
              if (it == m_ceBsrRxed.end ())
                {
                  // restart from the first
                  it = m_ceBsrRxed.begin ();
                }
              NS_LOG_DEBUG (this << " UE discarded for CQI = 0, RNTI " << uldci.m_rnti);
              // remove UE from allocation map
              for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
                {
                  rbgAllocationMap.at (i) = 0;
                }
              continue; // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
            }
          uldci.m_mcs = m_amc->GetMcsFromCqi (cqi);
        }

      uldci.m_tbSize = (m_amc->GetUlTbSizeFromMcs (uldci.m_mcs, rbPerFlow) / 8);
      UpdateUlRlcBufferInfo (uldci.m_rnti, uldci.m_tbSize);
      uldci.m_ndi = 1;
      uldci.m_cceIndex = 0;
      uldci.m_aggrLevel = 1;
      uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
      uldci.m_hopping = false;
      uldci.m_n2Dmrs = 0;
      uldci.m_tpc = 0; // no power control
      uldci.m_cqiRequest = false; // only period CQI at this stage
      uldci.m_ulIndex = 0; // TDD parameter
      uldci.m_dai = 1; // TDD parameter
      uldci.m_freqHopping = 0;
      uldci.m_pdcchPowerOffset = 0; // not used
      ret.m_dciList.push_back (uldci);
      // store DCI for HARQ_PERIOD
      uint8_t harqId = 0;
      if (m_harqOn == true)
        {
          std::map <uint16_t, uint8_t>::iterator itProcId;
          itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
          if (itProcId == m_ulHarqCurrentProcessId.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
            }
          harqId = (*itProcId).second;
          std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
          if (itDci == m_ulHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
            }
          (*itDci).second.at (harqId) = uldci;
          // Update HARQ process status (RV 0)
          std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (uldci.m_rnti);
          if (itStat == m_ulHarqProcessesStatus.end ())
            {
              NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << uldci.m_rnti);
            }
          (*itStat).second.at (harqId) = 0;
        }

      NS_LOG_INFO (this << " UE Allocation RNTI " << (*it).first << " startPRB " << (uint32_t)uldci.m_rbStart << " nPRB " << (uint32_t)uldci.m_rbLen << " CQI " << cqi << " MCS " << (uint32_t)uldci.m_mcs << " TBsize " << uldci.m_tbSize << " RbAlloc " << rbAllocated << " harqId " << (uint16_t)harqId);

      // update TTI  UE stats
      itStats = m_flowStatsUl.find ((*it).first);
      if (itStats != m_flowStatsUl.end ())
        {
          (*itStats).second.lastTtiBytesTransmitted =  uldci.m_tbSize;
        }
      else
        {
          NS_LOG_DEBUG (this << " No Stats for this allocated UE");
        }


      it++;
      if (it == m_ceBsrRxed.end ())
        {
          // restart from the first
          it = m_ceBsrRxed.begin ();
        }
      if ((rbAllocated == m_cschedCellConfig.m_ulBandwidth) || (rbPerFlow == 0))
        {
          // Stop allocation: no more PRBs
          m_nextRntiUl = (*it).first;
          break;
        }
    }
  while (((*it).first != m_nextRntiUl)&&(rbPerFlow!=0));


  // Update global UE stats
  // update UEs stats
  for (itStats = m_flowStatsUl.begin (); itStats != m_flowStatsUl.end (); itStats++)
    {
      (*itStats).second.totalBytesTransmitted += (*itStats).second.lastTtiBytesTransmitted;
      // update average throughput (see eq. 12.3 of Sec 12.3.1.2 of LTE – The UMTS Long Term Evolution, Ed Wiley)
      (*itStats).second.lastAveragedThroughput = ((1.0 - (1.0 / m_timeWindow)) * (*itStats).second.lastAveragedThroughput) + ((1.0 / m_timeWindow) * (double)((*itStats).second.lastTtiBytesTransmitted / 0.001));
      NS_LOG_INFO (this << " UE total bytes " << (*itStats).second.totalBytesTransmitted);
      NS_LOG_INFO (this << " UE average throughput " << (*itStats).second.lastAveragedThroughput);
      (*itStats).second.lastTtiBytesTransmitted = 0;
    }
  m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
  m_schedSapUser->SchedUlConfigInd (ret);

  return;
  }
}

void
RrFfMacScheduler::DoSchedUlNoiseInterferenceReq (const struct FfMacSchedSapProvider::SchedUlNoiseInterferenceReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  return;
}

void
RrFfMacScheduler::DoSchedUlSrInfoReq (const struct FfMacSchedSapProvider::SchedUlSrInfoReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  return;
}

void
RrFfMacScheduler::DoSchedUlMacCtrlInfoReq (const struct FfMacSchedSapProvider::SchedUlMacCtrlInfoReqParameters& params)
{
  if(m_flag == 0)
  {
    // std::cout << "执行0" << std::endl;
  NS_LOG_FUNCTION (this);

  std::map <uint16_t,uint32_t>::iterator it;

  for (unsigned int i = 0; i < params.m_macCeList.size (); i++)
    {
      if ( params.m_macCeList.at (i).m_macCeType == MacCeListElement_s::BSR )
        {
          // buffer status report
          // note that this scheduler does not differentiate the
          // allocation according to which LCGs have more/less bytes
          // to send.
          // Hence the BSR of different LCGs are just summed up to get
          // a total queue size that is used for allocation purposes.

          uint32_t buffer = 0;
          for (uint8_t lcg = 0; lcg < 4; ++lcg)
            {
              uint8_t bsrId = params.m_macCeList.at (i).m_macCeValue.m_bufferStatus.at (lcg);
              buffer += BufferSizeLevelBsr::BsrId2BufferSize (bsrId);
            }

          uint16_t rnti = params.m_macCeList.at (i).m_rnti;
          it = m_ceBsrRxed.find (rnti);
          if (it == m_ceBsrRxed.end ())
            {
              // create the new entry
              m_ceBsrRxed.insert ( std::pair<uint16_t, uint32_t > (rnti, buffer));
              NS_LOG_INFO (this << " Insert RNTI " << rnti << " queue " << buffer);
            }
          else
            {
              // update the buffer size value
              (*it).second = buffer;
              NS_LOG_INFO (this << " Update RNTI " << rnti << " queue " << buffer);
            }
        }
    }

  return;
  }
  else if(m_flag == 2)
  {
    NS_LOG_FUNCTION (this);

  std::map <uint16_t,uint32_t>::iterator it;

  for (unsigned int i = 0; i < params.m_macCeList.size (); i++)
    {
      if ( params.m_macCeList.at (i).m_macCeType == MacCeListElement_s::BSR )
        {
          // buffer status report
          // note that this scheduler does not differentiate the
          // allocation according to which LCGs have more/less bytes
          // to send.
          // Hence the BSR of different LCGs are just summed up to get
          // a total queue size that is used for allocation purposes.

          uint32_t buffer = 0;
          for (uint8_t lcg = 0; lcg < 4; ++lcg)
            {
              uint8_t bsrId = params.m_macCeList.at (i).m_macCeValue.m_bufferStatus.at (lcg);
              buffer += BufferSizeLevelBsr::BsrId2BufferSize (bsrId);
            }

          uint16_t rnti = params.m_macCeList.at (i).m_rnti;
          NS_LOG_LOGIC (this << "RNTI=" << rnti << " buffer=" << buffer);
          it = m_ceBsrRxed.find (rnti);
          if (it == m_ceBsrRxed.end ())
            {
              // create the new entry
              m_ceBsrRxed.insert ( std::pair<uint16_t, uint32_t > (rnti, buffer));
            }
          else
            {
              // update the buffer size value
              (*it).second = buffer;
            }
        }
    }

  return;
  }
  else if(m_flag == 3)
  {

  NS_LOG_FUNCTION (this);

  std::map <uint16_t,uint32_t>::iterator it;

  for (unsigned int i = 0; i < params.m_macCeList.size (); i++)
    {
      if ( params.m_macCeList.at (i).m_macCeType == MacCeListElement_s::BSR )
        {
          // buffer status report
          // note that this scheduler does not differentiate the
          // allocation according to which LCGs have more/less bytes
          // to send.
          // Hence the BSR of different LCGs are just summed up to get
          // a total queue size that is used for allocation purposes.

          uint32_t buffer = 0;
          for (uint8_t lcg = 0; lcg < 4; ++lcg)
            {
              uint8_t bsrId = params.m_macCeList.at (i).m_macCeValue.m_bufferStatus.at (lcg);
              buffer += BufferSizeLevelBsr::BsrId2BufferSize (bsrId);
            }
          
          uint16_t rnti = params.m_macCeList.at (i).m_rnti;
          NS_LOG_LOGIC (this << "RNTI=" << rnti << " buffer=" << buffer);
          it = m_ceBsrRxed.find (rnti);
          if (it == m_ceBsrRxed.end ())
            {
              // create the new entry
              m_ceBsrRxed.insert ( std::pair<uint16_t, uint32_t > (rnti, buffer));
            }
          else
            {
              // update the buffer size value
              (*it).second = buffer;
            }
        }
    }

  return;
  }
  else
  {
    // std::cout << "执行1" << std::endl;
    NS_LOG_FUNCTION (this);

  std::map <uint16_t,uint32_t>::iterator it;

  for (unsigned int i = 0; i < params.m_macCeList.size (); i++)
    {
      if ( params.m_macCeList.at (i).m_macCeType == MacCeListElement_s::BSR )
        {
          // buffer status report
          // note that this scheduler does not differentiate the
          // allocation according to which LCGs have more/less bytes
          // to send.
          // Hence the BSR of different LCGs are just summed up to get
          // a total queue size that is used for allocation purposes.

          uint32_t buffer = 0;
          for (uint8_t lcg = 0; lcg < 4; ++lcg)
            {
              uint8_t bsrId = params.m_macCeList.at (i).m_macCeValue.m_bufferStatus.at (lcg);
              buffer += BufferSizeLevelBsr::BsrId2BufferSize (bsrId);
            }

          uint16_t rnti = params.m_macCeList.at (i).m_rnti;
          NS_LOG_LOGIC (this << "RNTI=" << rnti << " buffer=" << buffer);
          it = m_ceBsrRxed.find (rnti);
          if (it == m_ceBsrRxed.end ())
            {
              // create the new entry
              m_ceBsrRxed.insert ( std::pair<uint16_t, uint32_t > (rnti, buffer));
            }
          else
            {
              // update the buffer size value
              (*it).second = buffer;
            }
        }
    }

  return;
  }
}

void
RrFfMacScheduler::DoSchedUlCqiInfoReq (const struct FfMacSchedSapProvider::SchedUlCqiInfoReqParameters& params)
{
  // if(m_flag == 0)
  // {
  //   std::cout << "执行0" << std::endl;
  // NS_LOG_FUNCTION (this);

  // switch (m_ulCqiFilter)
  //   {
  //   case FfMacScheduler::SRS_UL_CQI:
  //     {
  //       // filter all the CQIs that are not SRS based
  //       if (params.m_ulCqi.m_type != UlCqi_s::SRS)
  //         {
  //           return;
  //         }
  //     }
  //     break;
  //   case FfMacScheduler::PUSCH_UL_CQI:
  //     {
  //       // filter all the CQIs that are not SRS based
  //       if (params.m_ulCqi.m_type != UlCqi_s::PUSCH)
  //         {
  //           return;
  //         }
  //     }
  //     break;
  //   default:
  //     NS_FATAL_ERROR ("Unknown UL CQI type");
  //   }
  // switch (params.m_ulCqi.m_type)
  //   {
  //   case UlCqi_s::PUSCH:
  //     {
  //       std::map <uint16_t, std::vector <uint16_t> >::iterator itMap;
  //       std::map <uint16_t, std::vector <double> >::iterator itCqi;
  //       itMap = m_allocationMaps.find (params.m_sfnSf);
  //       if (itMap == m_allocationMaps.end ())
  //         {
  //           NS_LOG_INFO (this << " Does not find info on allocation, size : " << m_allocationMaps.size ());
  //           return;
  //         }
  //       for (uint32_t i = 0; i < (*itMap).second.size (); i++)
  //         {
  //           // convert from fixed point notation Sxxxxxxxxxxx.xxx to double
  //           double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (i));
  //           itCqi = m_ueCqi.find ((*itMap).second.at (i));
  //           if (itCqi == m_ueCqi.end ())
  //             {
  //               // create a new entry
  //               std::vector <double> newCqi;
  //               for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
  //                 {
  //                   if (i == j)
  //                     {
  //                       newCqi.push_back (sinr);
  //                     }
  //                   else
  //                     {
  //                       // initialize with NO_SINR value.
  //                       newCqi.push_back (30.0);
  //                     }

  //                 }
  //               m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > ((*itMap).second.at (i), newCqi));
  //               // generate correspondent timer
  //               m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > ((*itMap).second.at (i), m_cqiTimersThreshold));
  //             }
  //           else
  //             {
  //               // update the value
  //               (*itCqi).second.at (i) = sinr;
  //               // update correspondent timer
  //               std::map <uint16_t, uint32_t>::iterator itTimers;
  //               itTimers = m_ueCqiTimers.find ((*itMap).second.at (i));
  //               (*itTimers).second = m_cqiTimersThreshold;

  //             }

  //         }
  //       // remove obsolete info on allocation
  //       m_allocationMaps.erase (itMap);
  //     }
  //     break;
  //   case UlCqi_s::SRS:
  //     {
  //       // get the RNTI from vendor specific parameters
  //       uint16_t rnti = 0;
  //       NS_ASSERT (params.m_vendorSpecificList.size () > 0);
  //       for (uint16_t i = 0; i < params.m_vendorSpecificList.size (); i++)
  //         {
  //           if (params.m_vendorSpecificList.at (i).m_type == SRS_CQI_RNTI_VSP)
  //             {
  //               Ptr<SrsCqiRntiVsp> vsp = DynamicCast<SrsCqiRntiVsp> (params.m_vendorSpecificList.at (i).m_value);
  //               rnti = vsp->GetRnti ();
  //             }
  //         }
  //       std::map <uint16_t, std::vector <double> >::iterator itCqi;
  //       itCqi = m_ueCqi.find (rnti);
  //       if (itCqi == m_ueCqi.end ())
  //         {
  //           // create a new entry
  //           std::vector <double> newCqi;
  //           for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
  //             {
  //               double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
  //               newCqi.push_back (sinr);
  //               NS_LOG_INFO (this << " RNTI " << rnti << " new SRS-CQI for RB  " << j << " value " << sinr);

  //             }
  //           m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > (rnti, newCqi));
  //           // generate correspondent timer
  //           m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
  //         }
  //       else
  //         {
  //           // update the values
  //           for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
  //             {
  //               double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
  //               (*itCqi).second.at (j) = sinr;
  //               NS_LOG_INFO (this << " RNTI " << rnti << " update SRS-CQI for RB  " << j << " value " << sinr);
  //             }
  //           // update correspondent timer
  //           std::map <uint16_t, uint32_t>::iterator itTimers;
  //           itTimers = m_ueCqiTimers.find (rnti);
  //           (*itTimers).second = m_cqiTimersThreshold;

  //         }


  //     }
  //     break;
  //   case UlCqi_s::PUCCH_1:
  //   case UlCqi_s::PUCCH_2:
  //   case UlCqi_s::PRACH:
  //     {
  //       NS_FATAL_ERROR ("PfFfMacScheduler supports only PUSCH and SRS UL-CQIs");
  //     }
  //     break;
  //   default:
  //     NS_FATAL_ERROR ("Unknown type of UL-CQI");
  //   }
  // return;
  // }
  // else if(m_flag == 2)
  // {
  //   NS_LOG_FUNCTION (this);
  // // retrieve the allocation for this subframe
  // switch (m_ulCqiFilter)
  //   {
  //   case FfMacScheduler::SRS_UL_CQI:
  //     {
  //       // filter all the CQIs that are not SRS based
  //       if (params.m_ulCqi.m_type != UlCqi_s::SRS)
  //         {
  //           return;
  //         }
  //     }
  //     break;
  //   case FfMacScheduler::PUSCH_UL_CQI:
  //     {
  //       // filter all the CQIs that are not SRS based
  //       if (params.m_ulCqi.m_type != UlCqi_s::PUSCH)
  //         {
  //           return;
  //         }
  //     }
  //     break;
  //   default:
  //     NS_FATAL_ERROR ("Unknown UL CQI type");
  //   }

  // switch (params.m_ulCqi.m_type)
  //   {
  //   case UlCqi_s::PUSCH:
  //     {
  //       std::map <uint16_t, std::vector <uint16_t> >::iterator itMap;
  //       std::map <uint16_t, std::vector <double> >::iterator itCqi;
  //       NS_LOG_DEBUG (this << " Collect PUSCH CQIs of Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
  //       itMap = m_allocationMaps.find (params.m_sfnSf);
  //       if (itMap == m_allocationMaps.end ())
  //         {
  //           return;
  //         }
  //       for (uint32_t i = 0; i < (*itMap).second.size (); i++)
  //         {
  //           // convert from fixed point notation Sxxxxxxxxxxx.xxx to double
  //           double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (i));
  //           itCqi = m_ueCqi.find ((*itMap).second.at (i));
  //           if (itCqi == m_ueCqi.end ())
  //             {
  //               // create a new entry
  //               std::vector <double> newCqi;
  //               for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
  //                 {
  //                   if (i == j)
  //                     {
  //                       newCqi.push_back (sinr);
  //                     }
  //                   else
  //                     {
  //                       // initialize with NO_SINR value.
  //                       newCqi.push_back (NO_SINR);
  //                     }

  //                 }
  //               m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > ((*itMap).second.at (i), newCqi));
  //               // generate correspondent timer
  //               m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > ((*itMap).second.at (i), m_cqiTimersThreshold));
  //             }
  //           else
  //             {
  //               // update the value
  //               (*itCqi).second.at (i) = sinr;
  //               NS_LOG_DEBUG (this << " RNTI " << (*itMap).second.at (i) << " RB " << i << " SINR " << sinr);
  //               // update correspondent timer
  //               std::map <uint16_t, uint32_t>::iterator itTimers;
  //               itTimers = m_ueCqiTimers.find ((*itMap).second.at (i));
  //               (*itTimers).second = m_cqiTimersThreshold;

  //             }

  //         }
  //       // remove obsolete info on allocation
  //       m_allocationMaps.erase (itMap);
  //     }
  //     break;
  //   case UlCqi_s::SRS:
  //     {
  //       // get the RNTI from vendor specific parameters
  //       uint16_t rnti = 0;
  //       NS_ASSERT (params.m_vendorSpecificList.size () > 0);
  //       for (uint16_t i = 0; i < params.m_vendorSpecificList.size (); i++)
  //         {
  //           if (params.m_vendorSpecificList.at (i).m_type == SRS_CQI_RNTI_VSP)
  //             {
  //               Ptr<SrsCqiRntiVsp> vsp = DynamicCast<SrsCqiRntiVsp> (params.m_vendorSpecificList.at (i).m_value);
  //               rnti = vsp->GetRnti ();
  //             }
  //         }
  //       std::map <uint16_t, std::vector <double> >::iterator itCqi;
  //       itCqi = m_ueCqi.find (rnti);
  //       if (itCqi == m_ueCqi.end ())
  //         {
  //           // create a new entry
  //           std::vector <double> newCqi;
  //           for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
  //             {
  //               double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
  //               newCqi.push_back (sinr);
  //               NS_LOG_INFO (this << " RNTI " << rnti << " new SRS-CQI for RB  " << j << " value " << sinr);

  //             }
  //           m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > (rnti, newCqi));
  //           // generate correspondent timer
  //           m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
  //         }
  //       else
  //         {
  //           // update the values
  //           for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
  //             {
  //               double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
  //               (*itCqi).second.at (j) = sinr;
  //               NS_LOG_INFO (this << " RNTI " << rnti << " update SRS-CQI for RB  " << j << " value " << sinr);
  //             }
  //           // update correspondent timer
  //           std::map <uint16_t, uint32_t>::iterator itTimers;
  //           itTimers = m_ueCqiTimers.find (rnti);
  //           (*itTimers).second = m_cqiTimersThreshold;

  //         }


  //     }
  //     break;
  //   case UlCqi_s::PUCCH_1:
  //   case UlCqi_s::PUCCH_2:
  //   case UlCqi_s::PRACH:
  //     {
  //       NS_FATAL_ERROR ("FdBetFfMacScheduler supports only PUSCH and SRS UL-CQIs");
  //     }
  //     break;
  //   default:
  //     NS_FATAL_ERROR ("Unknown type of UL-CQI");
  //   }
  // return;
  // }
  // else if(m_flag == 3)
  // {

  // NS_LOG_FUNCTION (this);
  // // retrieve the allocation for this subframe
  // switch (m_ulCqiFilter)
  //   {
  //   case FfMacScheduler::SRS_UL_CQI:
  //     {
  //       // filter all the CQIs that are not SRS based
  //       if (params.m_ulCqi.m_type != UlCqi_s::SRS)
  //         {
  //           return;
  //         }
  //     }
  //     break;
  //   case FfMacScheduler::PUSCH_UL_CQI:
  //     {
  //       // filter all the CQIs that are not SRS based
  //       if (params.m_ulCqi.m_type != UlCqi_s::PUSCH)
  //         {
  //           return;
  //         }
  //     }
  //     break;
  //   default:
  //     NS_FATAL_ERROR ("Unknown UL CQI type");
  //   }

  // switch (params.m_ulCqi.m_type)
  //   {
  //   case UlCqi_s::PUSCH:
  //     {
  //       std::map <uint16_t, std::vector <uint16_t> >::iterator itMap;
  //       std::map <uint16_t, std::vector <double> >::iterator itCqi;
  //       NS_LOG_DEBUG (this << " Collect PUSCH CQIs of Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
  //       itMap = m_allocationMaps.find (params.m_sfnSf);
  //       if (itMap == m_allocationMaps.end ())
  //         {
  //           return;
  //         }
  //       for (uint32_t i = 0; i < (*itMap).second.size (); i++)
  //         {
  //           // convert from fixed point notation Sxxxxxxxxxxx.xxx to double
  //           double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (i));
  //           itCqi = m_ueCqi.find ((*itMap).second.at (i));
  //           if (itCqi == m_ueCqi.end ())
  //             {
  //               // create a new entry
  //               std::vector <double> newCqi;
  //               for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
  //                 {
  //                   if (i == j)
  //                     {
  //                       newCqi.push_back (sinr);
  //                     }
  //                   else
  //                     {
  //                       // initialize with NO_SINR value.
  //                       newCqi.push_back (NO_SINR);
  //                     }

  //                 }
  //               m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > ((*itMap).second.at (i), newCqi));
  //               // generate correspondent timer
  //               m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > ((*itMap).second.at (i), m_cqiTimersThreshold));
  //             }
  //           else
  //             {
  //               // update the value
  //               (*itCqi).second.at (i) = sinr;
  //               NS_LOG_DEBUG (this << " RNTI " << (*itMap).second.at (i) << " RB " << i << " SINR " << sinr);
  //               // update correspondent timer
  //               std::map <uint16_t, uint32_t>::iterator itTimers;
  //               itTimers = m_ueCqiTimers.find ((*itMap).second.at (i));
  //               (*itTimers).second = m_cqiTimersThreshold;

  //             }

  //         }
  //       // remove obsolete info on allocation
  //       m_allocationMaps.erase (itMap);
  //     }
  //     break;
  //   case UlCqi_s::SRS:
  //     {
  //       // get the RNTI from vendor specific parameters
  //       uint16_t rnti = 0;
  //       NS_ASSERT (params.m_vendorSpecificList.size () > 0);
  //       for (uint16_t i = 0; i < params.m_vendorSpecificList.size (); i++)
  //         {
  //           if (params.m_vendorSpecificList.at (i).m_type == SRS_CQI_RNTI_VSP)
  //             {
  //               Ptr<SrsCqiRntiVsp> vsp = DynamicCast<SrsCqiRntiVsp> (params.m_vendorSpecificList.at (i).m_value);
  //               rnti = vsp->GetRnti ();
  //             }
  //         }
  //       std::map <uint16_t, std::vector <double> >::iterator itCqi;
  //       itCqi = m_ueCqi.find (rnti);
  //       if (itCqi == m_ueCqi.end ())
  //         {
  //           // create a new entry
  //           std::vector <double> newCqi;
  //           for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
  //             {
  //               double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
  //               newCqi.push_back (sinr);
  //               NS_LOG_INFO (this << " RNTI " << rnti << " new SRS-CQI for RB  " << j << " value " << sinr);

  //             }
  //           m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > (rnti, newCqi));
  //           // generate correspondent timer
  //           m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
  //         }
  //       else
  //         {
  //           // update the values
  //           for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
  //             {
  //               double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
  //               (*itCqi).second.at (j) = sinr;
  //               NS_LOG_INFO (this << " RNTI " << rnti << " update SRS-CQI for RB  " << j << " value " << sinr);
  //             }
  //           // update correspondent timer
  //           std::map <uint16_t, uint32_t>::iterator itTimers;
  //           itTimers = m_ueCqiTimers.find (rnti);
  //           (*itTimers).second = m_cqiTimersThreshold;

  //         }


  //     }
  //     break;
  //   case UlCqi_s::PUCCH_1:
  //   case UlCqi_s::PUCCH_2:
  //   case UlCqi_s::PRACH:
  //     {
  //       NS_FATAL_ERROR ("PssFfMacScheduler supports only PUSCH and SRS UL-CQIs");
  //     }
  //     break;
  //   default:
  //     NS_FATAL_ERROR ("Unknown type of UL-CQI");
  //   }
  // return;
  // }
  // else
  // {
    // std::cout << "执行1" << std::endl;
    NS_LOG_FUNCTION (this);
  // retrieve the allocation for this subframe
  switch (m_ulCqiFilter)
    {
    case FfMacScheduler::SRS_UL_CQI:
      {
        // filter all the CQIs that are not SRS based
        if (params.m_ulCqi.m_type != UlCqi_s::SRS)
          {
            return;
          }
      }
      break;
    case FfMacScheduler::PUSCH_UL_CQI:
      {
        // filter all the CQIs that are not SRS based
        if (params.m_ulCqi.m_type != UlCqi_s::PUSCH)
          {
            return;
          }
      }
      break;
    default:
      NS_FATAL_ERROR ("Unknown UL CQI type");
    }

  switch (params.m_ulCqi.m_type)
    {
    case UlCqi_s::PUSCH:
      {
        std::map <uint16_t, std::vector <uint16_t> >::iterator itMap;
        std::map <uint16_t, std::vector <double> >::iterator itCqi;
        NS_LOG_DEBUG (this << " Collect PUSCH CQIs of Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
        itMap = m_allocationMaps.find (params.m_sfnSf);
        if (itMap == m_allocationMaps.end ())
          {
            return;
          }
        for (uint32_t i = 0; i < (*itMap).second.size (); i++)
          {
            // convert from fixed point notation Sxxxxxxxxxxx.xxx to double
            double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (i));
            itCqi = m_ueCqi.find ((*itMap).second.at (i));
            if (itCqi == m_ueCqi.end ())
              {
                // create a new entry
                std::vector <double> newCqi;
                for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
                  {
                    if (i == j)
                      {
                        newCqi.push_back (sinr);
                      }
                    else
                      {
                        // initialize with NO_SINR value.
                        newCqi.push_back (NO_SINR);
                      }

                  }
                m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > ((*itMap).second.at (i), newCqi));
                // generate correspondent timer
                m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > ((*itMap).second.at (i), m_cqiTimersThreshold));
              }
            else
              {
                // update the value
                (*itCqi).second.at (i) = sinr;
                NS_LOG_DEBUG (this << " RNTI " << (*itMap).second.at (i) << " RB " << i << " SINR " << sinr);
                // update correspondent timer
                std::map <uint16_t, uint32_t>::iterator itTimers;
                itTimers = m_ueCqiTimers.find ((*itMap).second.at (i));
                (*itTimers).second = m_cqiTimersThreshold;

              }

          }
        // remove obsolete info on allocation
        m_allocationMaps.erase (itMap);
      }
      break;
    case UlCqi_s::SRS:
      {
        NS_LOG_DEBUG (this << " Collect SRS CQIs of Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
        // get the RNTI from vendor specific parameters
        uint16_t rnti = 0;
        NS_ASSERT (params.m_vendorSpecificList.size () > 0);
        for (uint16_t i = 0; i < params.m_vendorSpecificList.size (); i++)
          {
            if (params.m_vendorSpecificList.at (i).m_type == SRS_CQI_RNTI_VSP)
              {
                Ptr<SrsCqiRntiVsp> vsp = DynamicCast<SrsCqiRntiVsp> (params.m_vendorSpecificList.at (i).m_value);
                rnti = vsp->GetRnti ();
              }
          }
        std::map <uint16_t, std::vector <double> >::iterator itCqi;
        itCqi = m_ueCqi.find (rnti);
        if (itCqi == m_ueCqi.end ())
          {
            // create a new entry
            std::vector <double> newCqi;
            for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
              {
                double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
                newCqi.push_back (sinr);
                NS_LOG_INFO (this << " RNTI " << rnti << " new SRS-CQI for RB  " << j << " value " << sinr);

              }
            m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > (rnti, newCqi));
            // generate correspondent timer
            m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
          }
        else
          {
            // update the values
            for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
              {
                double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
                (*itCqi).second.at (j) = sinr;
                NS_LOG_INFO (this << " RNTI " << rnti << " update SRS-CQI for RB  " << j << " value " << sinr);
              }
            // update correspondent timer
            std::map <uint16_t, uint32_t>::iterator itTimers;
            itTimers = m_ueCqiTimers.find (rnti);
            (*itTimers).second = m_cqiTimersThreshold;

          }


      }
      break;
    case UlCqi_s::PUCCH_1:
    case UlCqi_s::PUCCH_2:
    case UlCqi_s::PRACH:
      {
        NS_FATAL_ERROR ("PfFfMacScheduler supports only PUSCH and SRS UL-CQIs");
      }
      break;
    default:
      NS_FATAL_ERROR ("Unknown type of UL-CQI");
    }
  return;
  // }
  
}


void
RrFfMacScheduler::RefreshDlCqiMaps (void)
{
  // if(m_flag == 0)
  // {
  //   std::cout << "执行0" << std::endl;
  // NS_LOG_FUNCTION (this << m_p10CqiTimers.size ());
  // // refresh DL CQI P01 Map
  // std::map <uint16_t,uint32_t>::iterator itP10 = m_p10CqiTimers.begin ();
  // while (itP10 != m_p10CqiTimers.end ())
  //   {
  //     NS_LOG_INFO (this << " P10-CQI for user " << (*itP10).first << " is " << (uint32_t)(*itP10).second << " thr " << (uint32_t)m_cqiTimersThreshold);
  //     if ((*itP10).second == 0)
  //       {
  //         // delete correspondent entries
  //         std::map <uint16_t,uint8_t>::iterator itMap = m_p10CqiRxed.find ((*itP10).first);
  //         NS_ASSERT_MSG (itMap != m_p10CqiRxed.end (), " Does not find CQI report for user " << (*itP10).first);
  //         NS_LOG_INFO (this << " P10-CQI exired for user " << (*itP10).first);
  //         m_p10CqiRxed.erase (itMap);
  //         std::map <uint16_t,uint32_t>::iterator temp = itP10;
  //         itP10++;
  //         m_p10CqiTimers.erase (temp);
  //       }
  //     else
  //       {
  //         (*itP10).second--;
  //         itP10++;
  //       }
  //   }

  // return;
  // }
  // else if(m_flag == 2)
  // {
  //   // refresh DL CQI P01 Map
  // std::map <uint16_t,uint32_t>::iterator itP10 = m_p10CqiTimers.begin ();
  // while (itP10 != m_p10CqiTimers.end ())
  //   {
  //     NS_LOG_INFO (this << " P10-CQI for user " << (*itP10).first << " is " << (uint32_t)(*itP10).second << " thr " << (uint32_t)m_cqiTimersThreshold);
  //     if ((*itP10).second == 0)
  //       {
  //         // delete correspondent entries
  //         std::map <uint16_t,uint8_t>::iterator itMap = m_p10CqiRxed.find ((*itP10).first);
  //         NS_ASSERT_MSG (itMap != m_p10CqiRxed.end (), " Does not find CQI report for user " << (*itP10).first);
  //         NS_LOG_INFO (this << " P10-CQI expired for user " << (*itP10).first);
  //         m_p10CqiRxed.erase (itMap);
  //         std::map <uint16_t,uint32_t>::iterator temp = itP10;
  //         itP10++;
  //         m_p10CqiTimers.erase (temp);
  //       }
  //     else
  //       {
  //         (*itP10).second--;
  //         itP10++;
  //       }
  //   }

  // // refresh DL CQI A30 Map
  // std::map <uint16_t,uint32_t>::iterator itA30 = m_a30CqiTimers.begin ();
  // while (itA30 != m_a30CqiTimers.end ())
  //   {
  //     NS_LOG_INFO (this << " A30-CQI for user " << (*itA30).first << " is " << (uint32_t)(*itA30).second << " thr " << (uint32_t)m_cqiTimersThreshold);
  //     if ((*itA30).second == 0)
  //       {
  //         // delete correspondent entries
  //         std::map <uint16_t,SbMeasResult_s>::iterator itMap = m_a30CqiRxed.find ((*itA30).first);
  //         NS_ASSERT_MSG (itMap != m_a30CqiRxed.end (), " Does not find CQI report for user " << (*itA30).first);
  //         NS_LOG_INFO (this << " A30-CQI expired for user " << (*itA30).first);
  //         m_a30CqiRxed.erase (itMap);
  //         std::map <uint16_t,uint32_t>::iterator temp = itA30;
  //         itA30++;
  //         m_a30CqiTimers.erase (temp);
  //       }
  //     else
  //       {
  //         (*itA30).second--;
  //         itA30++;
  //       }
  //   }

  // return;
  // }
  // else if(m_flag == 3)
  // {
  //   // refresh DL CQI P01 Map
  // std::map <uint16_t,uint32_t>::iterator itP10 = m_p10CqiTimers.begin ();
  // while (itP10 != m_p10CqiTimers.end ())
  //   {
  //     NS_LOG_INFO (this << " P10-CQI for user " << (*itP10).first << " is " << (uint32_t)(*itP10).second << " thr " << (uint32_t)m_cqiTimersThreshold);
  //     if ((*itP10).second == 0)
  //       {
  //         // delete correspondent entries
  //         std::map <uint16_t,uint8_t>::iterator itMap = m_p10CqiRxed.find ((*itP10).first);
  //         NS_ASSERT_MSG (itMap != m_p10CqiRxed.end (), " Does not find CQI report for user " << (*itP10).first);
  //         NS_LOG_INFO (this << " P10-CQI expired for user " << (*itP10).first);
  //         m_p10CqiRxed.erase (itMap);
  //         std::map <uint16_t,uint32_t>::iterator temp = itP10;
  //         itP10++;
  //         m_p10CqiTimers.erase (temp);
  //       }
  //     else
  //       {
  //         (*itP10).second--;
  //         itP10++;
  //       }
  //   }

  // // refresh DL CQI A30 Map
  // std::map <uint16_t,uint32_t>::iterator itA30 = m_a30CqiTimers.begin ();
  // while (itA30 != m_a30CqiTimers.end ())
  //   {
  //     NS_LOG_INFO (this << " A30-CQI for user " << (*itA30).first << " is " << (uint32_t)(*itA30).second << " thr " << (uint32_t)m_cqiTimersThreshold);
  //     if ((*itA30).second == 0)
  //       {
  //         // delete correspondent entries
  //         std::map <uint16_t,SbMeasResult_s>::iterator itMap = m_a30CqiRxed.find ((*itA30).first);
  //         NS_ASSERT_MSG (itMap != m_a30CqiRxed.end (), " Does not find CQI report for user " << (*itA30).first);
  //         NS_LOG_INFO (this << " A30-CQI expired for user " << (*itA30).first);
  //         m_a30CqiRxed.erase (itMap);
  //         std::map <uint16_t,uint32_t>::iterator temp = itA30;
  //         itA30++;
  //         m_a30CqiTimers.erase (temp);
  //       }
  //     else
  //       {
  //         (*itA30).second--;
  //         itA30++;
  //       }
  //   }

  // return;
  // }
  // else
  // {
    // std::cout << "执行1" << std::endl;
    // refresh DL CQI P01 Map
  std::map <uint16_t,uint32_t>::iterator itP10 = m_p10CqiTimers.begin ();
  while (itP10 != m_p10CqiTimers.end ())
    {
      NS_LOG_INFO (this << " P10-CQI for user " << (*itP10).first << " is " << (uint32_t)(*itP10).second << " thr " << (uint32_t)m_cqiTimersThreshold);
      if ((*itP10).second == 0)
        {
          // delete correspondent entries
          std::map <uint16_t,uint8_t>::iterator itMap = m_p10CqiRxed.find ((*itP10).first);
          NS_ASSERT_MSG (itMap != m_p10CqiRxed.end (), " Does not find CQI report for user " << (*itP10).first);
          NS_LOG_INFO (this << " P10-CQI expired for user " << (*itP10).first);
          m_p10CqiRxed.erase (itMap);
          std::map <uint16_t,uint32_t>::iterator temp = itP10;
          itP10++;
          m_p10CqiTimers.erase (temp);
        }
      else
        {
          (*itP10).second--;
          itP10++;
        }
    }

  // refresh DL CQI A30 Map
  std::map <uint16_t,uint32_t>::iterator itA30 = m_a30CqiTimers.begin ();
  while (itA30 != m_a30CqiTimers.end ())
    {
      NS_LOG_INFO (this << " A30-CQI for user " << (*itA30).first << " is " << (uint32_t)(*itA30).second << " thr " << (uint32_t)m_cqiTimersThreshold);
      if ((*itA30).second == 0)
        {
          // delete correspondent entries
          std::map <uint16_t,SbMeasResult_s>::iterator itMap = m_a30CqiRxed.find ((*itA30).first);
          NS_ASSERT_MSG (itMap != m_a30CqiRxed.end (), " Does not find CQI report for user " << (*itA30).first);
          NS_LOG_INFO (this << " A30-CQI expired for user " << (*itA30).first);
          m_a30CqiRxed.erase (itMap);
          std::map <uint16_t,uint32_t>::iterator temp = itA30;
          itA30++;
          m_a30CqiTimers.erase (temp);
        }
      else
        {
          (*itA30).second--;
          itA30++;
        }
    }

  return;
  // }
}


void
RrFfMacScheduler::RefreshUlCqiMaps (void)
{
  // refresh UL CQI  Map
  std::map <uint16_t,uint32_t>::iterator itUl = m_ueCqiTimers.begin ();
  while (itUl != m_ueCqiTimers.end ())
    {
      NS_LOG_INFO (this << " UL-CQI for user " << (*itUl).first << " is " << (uint32_t)(*itUl).second << " thr " << (uint32_t)m_cqiTimersThreshold);
      if ((*itUl).second == 0)
        {
          // delete correspondent entries
          std::map <uint16_t, std::vector <double> >::iterator itMap = m_ueCqi.find ((*itUl).first);
          NS_ASSERT_MSG (itMap != m_ueCqi.end (), " Does not find CQI report for user " << (*itUl).first);
          NS_LOG_INFO (this << " UL-CQI exired for user " << (*itUl).first);
          (*itMap).second.clear ();
          m_ueCqi.erase (itMap);
          std::map <uint16_t,uint32_t>::iterator temp = itUl;
          itUl++;
          m_ueCqiTimers.erase (temp);
        }
      else
        {
          (*itUl).second--;
          itUl++;
        }
    }

  return;
}

void
RrFfMacScheduler::UpdateDlRlcBufferInfo (uint16_t rnti, uint8_t lcid, uint16_t size)
{
  if(m_flag == 0)
  {
    // std::cout << "执行0" << std::endl;
  NS_LOG_FUNCTION (this);
  std::list<FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
  for (it = m_rlcBufferReq.begin (); it != m_rlcBufferReq.end (); it++)
    {
      if (((*it).m_rnti == rnti) && ((*it).m_logicalChannelIdentity == lcid))
        {
          NS_LOG_INFO (this << " UE " << rnti << " LC " << (uint16_t)lcid << " txqueue " << (*it).m_rlcTransmissionQueueSize << " retxqueue " << (*it).m_rlcRetransmissionQueueSize << " status " << (*it).m_rlcStatusPduSize << " decrease " << size);
          // Update queues: RLC tx order Status, ReTx, Tx
          // Update status queue
           if (((*it).m_rlcStatusPduSize > 0) && (size >= (*it).m_rlcStatusPduSize))
              {
                (*it).m_rlcStatusPduSize = 0;
              }
            else if (((*it).m_rlcRetransmissionQueueSize > 0) && (size >= (*it).m_rlcRetransmissionQueueSize))
              {
                (*it).m_rlcRetransmissionQueueSize = 0;
              }
            else if ((*it).m_rlcTransmissionQueueSize > 0)
              {
                uint32_t rlcOverhead;
                if (lcid == 1)
                  {
                    // for SRB1 (using RLC AM) it's better to
                    // overestimate RLC overhead rather than
                    // underestimate it and risk unneeded
                    // segmentation which increases delay 
                    rlcOverhead = 4;                                  
                  }
                else
                  {
                    // minimum RLC overhead due to header
                    rlcOverhead = 2;
                  }
                // update transmission queue
                if ((*it).m_rlcTransmissionQueueSize <= size - rlcOverhead)
                  {
                    (*it).m_rlcTransmissionQueueSize = 0;
                  }
                else
                  {                    
                    (*it).m_rlcTransmissionQueueSize -= size - rlcOverhead;
                  }
              }
          // return;
        }
    }
  }
  else
  {
    std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it1;
  LteFlowId_t flow (rnti, lcid);
  it1 = m_rlcBufferReq_1.find (flow);
  if (it1 != m_rlcBufferReq_1.end ())
    {
      NS_LOG_INFO (this << " UE " << rnti << " LC " << (uint16_t)lcid << " txqueue " << (*it1).second.m_rlcTransmissionQueueSize << " retxqueue " << (*it1).second.m_rlcRetransmissionQueueSize << " status " << (*it1).second.m_rlcStatusPduSize << " decrease " << size);
      // Update queues: RLC tx order Status, ReTx, Tx
      // Update status queue
      if (((*it1).second.m_rlcStatusPduSize > 0) && (size >= (*it1).second.m_rlcStatusPduSize))
        {
          (*it1).second.m_rlcStatusPduSize = 0;
        }
      else if (((*it1).second.m_rlcRetransmissionQueueSize > 0) && (size >= (*it1).second.m_rlcRetransmissionQueueSize))
        {
          (*it1).second.m_rlcRetransmissionQueueSize = 0;
        }
      else if ((*it1).second.m_rlcTransmissionQueueSize > 0)
        {
          uint32_t rlcOverhead;
          if (lcid == 1)
            {
              // for SRB1 (using RLC AM) it's better to
              // overestimate RLC overhead rather than
              // underestimate it and risk unneeded
              // segmentation which increases delay 
              rlcOverhead = 4;
            }
          else
            {
              // minimum RLC overhead due to header
              rlcOverhead = 2;
            }
          // update transmission queue
          if ((*it1).second.m_rlcTransmissionQueueSize <= size - rlcOverhead)
            {
              (*it1).second.m_rlcTransmissionQueueSize = 0;
            }
          else
            {
              (*it1).second.m_rlcTransmissionQueueSize -= size - rlcOverhead;
            }
        }
    }
  else
    {
      NS_LOG_ERROR (this << " Does not find DL RLC Buffer Report of UE " << rnti);
    }
  // }
  // else if(m_flag == 3)
  // {

  // std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
  // LteFlowId_t flow (rnti, lcid);
  // it = m_rlcBufferReq_1.find (flow);
  // if (it != m_rlcBufferReq_1.end ())
  //   {
  //     NS_LOG_INFO (this << " UE " << rnti << " LC " << (uint16_t)lcid << " txqueue " << (*it).second.m_rlcTransmissionQueueSize << " retxqueue " << (*it).second.m_rlcRetransmissionQueueSize << " status " << (*it).second.m_rlcStatusPduSize << " decrease " << size);
  //     // Update queues: RLC tx order Status, ReTx, Tx
  //     // Update status queue
  //     if (((*it).second.m_rlcStatusPduSize > 0) && (size >= (*it).second.m_rlcStatusPduSize))
  //       {
  //          (*it).second.m_rlcStatusPduSize = 0;
  //       }
  //     else if (((*it).second.m_rlcRetransmissionQueueSize > 0) && (size >= (*it).second.m_rlcRetransmissionQueueSize))
  //       {
  //         (*it).second.m_rlcRetransmissionQueueSize = 0;
  //       }
  //     else if ((*it).second.m_rlcTransmissionQueueSize > 0)
  //       {
  //         uint32_t rlcOverhead;
  //         if (lcid == 1)
  //           {
  //             // for SRB1 (using RLC AM) it's better to
  //             // overestimate RLC overhead rather than
  //             // underestimate it and risk unneeded
  //             // segmentation which increases delay 
  //             rlcOverhead = 4;                                  
  //           }
  //         else
  //           {
  //             // minimum RLC overhead due to header
  //             rlcOverhead = 2;
  //           }
  //         // update transmission queue
  //         if ((*it).second.m_rlcTransmissionQueueSize <= size - rlcOverhead)
  //           {
  //             (*it).second.m_rlcTransmissionQueueSize = 0;
  //           }
  //         else
  //           {                    
  //             (*it).second.m_rlcTransmissionQueueSize -= size - rlcOverhead;
  //           }
  //       }
  //   }
  // else
  //   {
  //     NS_LOG_ERROR (this << " Does not find DL RLC Buffer Report of UE " << rnti);
  //   }
  // }
  // else
  // {
  //   std::cout << "执行1" << std::endl;
  //   std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
  // LteFlowId_t flow (rnti, lcid);
  // it = m_rlcBufferReq_1.find (flow);
  // if (it != m_rlcBufferReq_1.end ())
  //   {
  //     NS_LOG_INFO (this << " UE " << rnti << " LC " << (uint16_t)lcid << " txqueue " << (*it).second.m_rlcTransmissionQueueSize << " retxqueue " << (*it).second.m_rlcRetransmissionQueueSize << " status " << (*it).second.m_rlcStatusPduSize << " decrease " << size);
  //     // Update queues: RLC tx order Status, ReTx, Tx
  //     // Update status queue
  //     if (((*it).second.m_rlcStatusPduSize > 0) && (size >= (*it).second.m_rlcStatusPduSize))
  //       {
  //         (*it).second.m_rlcStatusPduSize = 0;
  //       }
  //     else if (((*it).second.m_rlcRetransmissionQueueSize > 0) && (size >= (*it).second.m_rlcRetransmissionQueueSize))
  //       {
  //         (*it).second.m_rlcRetransmissionQueueSize = 0;
  //       }
  //     else if ((*it).second.m_rlcTransmissionQueueSize > 0)
  //       {
  //         uint32_t rlcOverhead;
  //         if (lcid == 1)
  //           {
  //             // for SRB1 (using RLC AM) it's better to
  //             // overestimate RLC overhead rather than
  //             // underestimate it and risk unneeded
  //             // segmentation which increases delay 
  //             rlcOverhead = 4;
  //           }
  //         else
  //           {
  //             // minimum RLC overhead due to header
  //             rlcOverhead = 2;
  //           }
  //         // update transmission queue
  //         if ((*it).second.m_rlcTransmissionQueueSize <= size - rlcOverhead)
  //           {
  //             (*it).second.m_rlcTransmissionQueueSize = 0;
  //           }
  //         else
  //           {
  //             (*it).second.m_rlcTransmissionQueueSize -= size - rlcOverhead;
  //           }
  //       }
  //   }
  // else
  //   {
  //     NS_LOG_ERROR (this << " Does not find DL RLC Buffer Report of UE " << rnti);
  //   }
  }
}

void
RrFfMacScheduler::UpdateUlRlcBufferInfo (uint16_t rnti, uint16_t size)
{

  size = size - 2; // remove the minimum RLC overhead
  std::map <uint16_t,uint32_t>::iterator it = m_ceBsrRxed.find (rnti);
  if (it != m_ceBsrRxed.end ())
    {
      NS_LOG_INFO (this << " Update RLC BSR UE " << rnti << " size " << size << " BSR " << (*it).second);
      if ((*it).second >= size)
        {
          (*it).second -= size;
        }
      else
        {
          (*it).second = 0;
        }
    }
  else
    {
      NS_LOG_ERROR (this << " Does not find BSR report info of UE " << rnti);
    }

}


void
RrFfMacScheduler::TransmissionModeConfigurationUpdate (uint16_t rnti, uint8_t txMode)
{
  NS_LOG_FUNCTION (this << " RNTI " << rnti << " txMode " << (uint16_t)txMode);
  FfMacCschedSapUser::CschedUeConfigUpdateIndParameters params;
  params.m_rnti = rnti;
  params.m_transmissionMode = txMode;
  m_cschedSapUser->CschedUeConfigUpdateInd (params);
}



}
