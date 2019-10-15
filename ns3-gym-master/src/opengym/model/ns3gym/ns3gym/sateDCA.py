##环境类
# -*- coding: utf-8 -*-
"""
Created on Sun Mar 31 16:53:22 2019

@author: Liqi
"""

import math
import gym
from gym import spaces
from gym.utils import seeding
import numpy as np


class sateDCA_ENV(gym.Env):
#    metadata = {
#        'render.modes': ['human', 'rgb_array'],
#        'video.frames_per_second' : 50
#    }
    
    def __init__(self):
        self.numOfBeam=7      #波束个数
        self.numOfChannel=12   #信道个数
        
        
        
        
        
        self.rOfBeam  = 1
        self.numOfUe = 1000
        self.matrixOfBeamLoca = self.Init_BeamLocal(self.numOfBeam)                       # 获取波束之间的位置关系
        self.matrixOfBeamDis  = self.Cal_DistanceOfBeam(self.rOfBeam, self.matrixOfBeamLoca)   # 计算波束之间的距离
        self.matrixOfBeamDis[np.where(self.matrixOfBeamDis == 0)] = 100                        # 将本波束的距离置为100是方便后续计算同频约束
        self.beamOfDistance = self.matrixOfBeamDis
         
        self.beamChanAlloc  = np.zeros((self.numOfBeam, self.numOfChannel))               #初始化信道占用矩阵
#        self.beamChanAlloc  =  np.array([[-1. , 1. , 0.,-1.,-1., 0., -1.,  0., -1., -1.,  1., -1., -1., 1., 0., -1., -1., -1., -1.,  0.,-1., -1.,  0.,-1.],\
#                                         [ 1. ,-1. ,-1., 1., 1., 0., -1., -1.,  1., -1., -1., -1., -1.,-1.,-1., -1.,  1., -1., -1.,  0.,-1., -1., -1.,-1.],\
#                                         [-1. ,-1. ,-1.,-1.,-1., 0.,  0., -1., -1., -1., -1.,  0., -1.,-1.,-1.,  1., -1.,  0., -1.,  0., 1.,  0., -1., 1.],\
#                                         [ 0. ,-1. ,-1.,-1., 0., 0.,  0.,  0.,  0.,  1., -1., -1., -1.,-1.,-1., -1., -1., -1., -1., -1.,-1., -1.,  0.,-1.],\
#                                         [-1. ,-1. , 0.,-1.,-1., 0.,  0., -1.,  0., -1., -1.,  1., -1.,-1., 0.,  0., -1.,  1.,  1.,  0., 0., -1.,  0.,-1.],\
#                                         [ 1. ,-1. , 0.,-1., 1., 0., -1., -1.,  0., -1., -1., -1.,  1.,-1., 0.,  0., -1., -1., -1., -1.,-1.,  1.,  0.,-1.],\
#                                         [-1. ,-1. , 0.,-1.,-1., 0.,  1.,  0., -1., -1., -1.,  0., -1.,-1., 0.,  0., -1., -1., -1., -1.,-1., -1., -1., 0.],\
#                                         [-1. , 0. ,-1.,-1.,-1., 0.,  0., -1., -1.,  1.,  1.,  1., -1., 0.,-1.,  0., -1.,  1., -1.,  0., 0., -1., -1., 0.],\
#                                         [-1. , 0. , 1.,-1.,-1., 0.,  0.,  1., -1., -1., -1., -1.,  1.,-1., 1., -1., -1., -1.,  1.,  0.,-1.,  0.,  1.,-1.],\
#                                         [ 0. , 0. ,-1., 0., 0., 0.,  0., -1.,  0.,  1.,  0.,  0., -1., 1.,-1., -1.,  0.,  0., -1.,  0.,-1.,  0., -1.,-1.],\
#                                         [ 0. , 0. ,-1.,-1., 0., 0.,  0.,  0.,  0., -1.,  0., -1.,  0.,-1.,-1., -1., -1.,  0.,  1., -1.,-1.,  0.,  0.,-1.],\
#                                         [ 0. , 0. , 1., 1., 0., 0.,  0.,  0.,  0., -1., -1.,  1., -1., 0., 1.,  0.,  1.,  0., -1.,  1., 0., -1.,  0.,-1.],\
#                                         [ 0. , 0. ,-1.,-1., 0., 0.,  0.,  0.,  0., -1.,  1., -1.,  1., 0.,-1.,  0., -1., -1., -1., -1., 0.,  1.,  0., 1.],\
#                                         [ 0. , 0. , 0., 1., 0., 0.,  0., -1.,  0.,  0., -1., -1., -1., 0., 0.,  0.,  1., -1., -1.,  0., 0., -1.,  0.,-1.],\
#                                         [-1. , 0. , 0.,-1.,-1., 0.,  0.,  1.,  0., -1.,  0., -1., -1., 0., 0.,  0., -1., -1., -1.,  0., 0., -1.,  0., 1.],\
#                                         [-1. , 0. , 0., 1.,-1., 0.,  0., -1.,  0.,  1., -1.,  0., -1., 0., 0.,  0., -1., -1.,  0., -1.,-1., -1.,  0.,-1.],\
#                                         [-1. , 0. , 0.,-1.,-1., 0., -1.,  0.,  0., -1.,  1.,  0., -1.,-1., 0.,  0.,  1.,  1., -1.,  1., 1., -1., -1., 0.],\
#                                         [ 0. , 0. , 0., 1., 0., 0., -1.,  0.,  0.,  1., -1.,  0.,  0., 1., 0.,  0., -1., -1.,  1., -1.,-1., -1.,  1., 0.],\
#                                         [-1. , 0. , 0.,-1.,-1., 0., -1.,  0., -1., -1., -1., -1.,  0.,-1., 0.,  0., -1., -1., -1.,  0., 0.,  1., -1., 0.]])
#        
#        ss = np.ones((self.numOfBeam, self.numOfChannel))*(-1)
#        
#        x = np.array([1,8,10,12,14,16,18])-1
#        y = np.array([1,9,11,13,15,17,19])-1
#        w = np.array([2,4,6,10,14,18])-1
#        z = np.array([3,5,7,8,12,16])-1
#        jj = 0
#        for k in range(6):
#            ss[x,jj] = 0
#            jj +=4
#            
#        jj = 1
#        for k in range(6):
#            ss[y,jj] = 0
#            jj +=4
#        jj = 2
#        for k in range(6):
#            ss[w,jj] = 0
#            jj +=4
#            
#        jj = 3
#        for k in range(6):
#            ss[z,jj] = 0
#            jj +=4
#        
#        self.beamChanAlloc  = ss;
        
#        self.observation_space = 3**(self.numOfBeam*self.numOfChannel)
#        self.action_space
        
        
        
        beamOfUser = np.zeros((1,self.numOfBeam))
        beamOfUser = np.array([15,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5])*100  
        self.beamOfUser = beamOfUser.copy()
        self.User           =  beamOfUser.copy()
        
        
        
        
        
    def _seed(self, seed=None):
        self.np_random, seed = seeding.np_random(seed)
        return [seed]
    
    
    
    
    def _step(self, action):
        #action是一个二元组(k,a)  k为用户所在的波束号，a为请求分配的信道号
        reward = 1.0
        nOfBeam = int(action/self.numOfChannel)
#        nOfBeam = action[0]
        nOfChannel = int(action%self.numOfChannel)
#        nOfChannel = action[1]
        self.beamOfUser[nOfBeam] -= 1
#        tp1 = np.where(self.beamChanAlloc == -1)[0].size
        self.beamChanAlloc[nOfBeam,nOfChannel] = 1
#        reward -= sum(self.beamOfUser)
        state = self.beamChanAlloc
        terminal_flag = False
        nextstate  =  self.Cal_BeamValidChan(state, self.beamOfDistance)
#        tp2 = np.where(nextstate == -1)[0].size
#        reward -= (tp2-tp1)
        if (nextstate != 0).all() :
            terminal_flag = True
        
        return nextstate,reward,terminal_flag,{}
    
    
    
    def _reset(self):
       
       
#        self.beamChanAlloc  =  np.array([[-1. , 1. , 0.,-1.,-1., 0., -1.,  0., -1., -1.,  1., -1., -1., 1., 0., -1., -1., -1., -1.,  0.,-1., -1.,  0.,-1.],\
#                                         [ 1. ,-1. ,-1., 1., 1., 0., -1., -1.,  1., -1., -1., -1., -1.,-1.,-1., -1.,  1., -1., -1.,  0.,-1., -1., -1.,-1.],\
#                                         [-1. ,-1. ,-1.,-1.,-1., 0.,  0., -1., -1., -1., -1.,  0., -1.,-1.,-1.,  1., -1.,  0., -1.,  0., 1.,  0., -1., 1.],\
#                                         [ 0. ,-1. ,-1.,-1., 0., 0.,  0.,  0.,  0.,  1., -1., -1., -1.,-1.,-1., -1., -1., -1., -1., -1.,-1., -1.,  0.,-1.],\
#                                         [-1. ,-1. , 0.,-1.,-1., 0.,  0., -1.,  0., -1., -1.,  1., -1.,-1., 0.,  0., -1.,  1.,  1.,  0., 0., -1.,  0.,-1.],\
#                                         [ 1. ,-1. , 0.,-1., 1., 0., -1., -1.,  0., -1., -1., -1.,  1.,-1., 0.,  0., -1., -1., -1., -1.,-1.,  1.,  0.,-1.],\
#                                         [-1. ,-1. , 0.,-1.,-1., 0.,  1.,  0., -1., -1., -1.,  0., -1.,-1., 0.,  0., -1., -1., -1., -1.,-1., -1., -1., 0.],\
#                                         [-1. , 0. ,-1.,-1.,-1., 0.,  0., -1., -1.,  1.,  1.,  1., -1., 0.,-1.,  0., -1.,  1., -1.,  0., 0., -1., -1., 0.],\
#                                         [-1. , 0. , 1.,-1.,-1., 0.,  0.,  1., -1., -1., -1., -1.,  1.,-1., 1., -1., -1., -1.,  1.,  0.,-1.,  0.,  1.,-1.],\
#                                         [ 0. , 0. ,-1., 0., 0., 0.,  0., -1.,  0.,  1.,  0.,  0., -1., 1.,-1., -1.,  0.,  0., -1.,  0.,-1.,  0., -1.,-1.],\
#                                         [ 0. , 0. ,-1.,-1., 0., 0.,  0.,  0.,  0., -1.,  0., -1.,  0.,-1.,-1., -1., -1.,  0.,  1., -1.,-1.,  0.,  0.,-1.],\
#                                         [ 0. , 0. , 1., 1., 0., 0.,  0.,  0.,  0., -1., -1.,  1., -1., 0., 1.,  0.,  1.,  0., -1.,  1., 0., -1.,  0.,-1.],\
#                                         [ 0. , 0. ,-1.,-1., 0., 0.,  0.,  0.,  0., -1.,  1., -1.,  1., 0.,-1.,  0., -1., -1., -1., -1., 0.,  1.,  0., 1.],\
#                                         [ 0. , 0. , 0., 1., 0., 0.,  0., -1.,  0.,  0., -1., -1., -1., 0., 0.,  0.,  1., -1., -1.,  0., 0., -1.,  0.,-1.],\
#                                         [-1. , 0. , 0.,-1.,-1., 0.,  0.,  1.,  0., -1.,  0., -1., -1., 0., 0.,  0., -1., -1., -1.,  0., 0., -1.,  0., 1.],\
#                                         [-1. , 0. , 0., 1.,-1., 0.,  0., -1.,  0.,  1., -1.,  0., -1., 0., 0.,  0., -1., -1.,  0., -1.,-1., -1.,  0.,-1.],\
#                                         [-1. , 0. , 0.,-1.,-1., 0., -1.,  0.,  0., -1.,  1.,  0., -1.,-1., 0.,  0.,  1.,  1., -1.,  1., 1., -1., -1., 0.],\
#                                         [ 0. , 0. , 0., 1., 0., 0., -1.,  0.,  0.,  1., -1.,  0.,  0., 1., 0.,  0., -1., -1.,  1., -1.,-1., -1.,  1., 0.],\
#                                         [-1. , 0. , 0.,-1.,-1., 0., -1.,  0., -1., -1., -1., -1.,  0.,-1., 0.,  0., -1., -1., -1.,  0., 0.,  1., -1., 0.]])
##    
       
#        self.beamChanAlloc  = np.array([ 0.,  1.,  1.,  0.,  0.,  0.,  0., -1.,  1., -1.,  0., -1., -1.,
#       -1., -1.,  0., -1., -1., -1.,  0.,  0.,  0., -1.,  0., -1.,  1.,
#        0., -1.,  1.,  0., -1.,  0., -1., -1., -1., -1.,  0., -1., -1.,
#        0., -1., -1.,  0.,  0., -1., -1., -1.,  0., -1., -1., -1.,  0.,
#       -1.,  0.,  0., -1., -1.,  0.,  0., -1., -1., -1.,  1.,  0.,  0.,
#       -1., -1., -1., -1.,  0., -1.,  1., -1., -1., -1.,  1.,  1., -1.,
#       -1., -1.,  0., -1., -1.,  0., -1., -1., -1., -1., -1., -1., -1.,
#       -1., -1.,  1., -1., -1., -1., -1., -1.,  0., -1.,  0., -1.,  0.,
#       -1., -1., -1., -1., -1., -1.,  1.,  0., -1.,  0.,  0.,  0.,  0.,
#        0., -1.,  0.,  0., -1.,  0.,  1., -1.,  0., -1.,  0.,  0., -1.,
#       -1., -1.,  0., -1.,  1.,  0.,  0., -1.,  0., -1., -1., -1.,  1.,
#        0., -1.,  1.,  1.,  1.,  0.,  1., -1.,  0.,  0.,  1.,  0.,  0.,
#        0.,  1., -1.,  0.,  1., -1., -1., -1., -1., -1.,  0., -1.,  0.,
#       -1.,  0.,  0.,  0., -1., -1.,  0., -1.,  0.,  0.,  0.,  1.,  0.,
#        0.,  1.,  0.,  0.,  0.,  0.,  0.,  1., -1.,  0.,  0., -1., -1.,
#       -1., -1.,  0.,  0., -1.,  0.,  0., -1., -1., -1., -1., -1., -1.,
#        0.,  1.,  1.,  1., -1.,  0., -1., -1.,  0., -1.,  1., -1., -1.,
#        1.,  1.,  1.,  0., -1., -1., -1.,  1., -1.,  1., -1., -1.,  1.,
#       -1., -1., -1., -1., -1., -1.,  0.,  1.,  1.,  0., -1.,  1., -1.,
#        1.,  1., -1., -1., -1., -1., -1.,  0.,  1.,  0., -1., -1.,  0.,
#       -1., -1.,  0., -1., -1.,  0.,  1.,  1.,  1., -1., -1., -1., -1.,
#        0.,  0.,  0.,  1.,  0., -1.,  0.,  0.,  0., -1., -1., -1.,  1.,
#       -1.,  0.,  1.,  0.,  0.,  0., -1.,  0.,  1.,  0.,  0., -1.,  0.,
#       -1., -1., -1., -1.,  0.]).reshape(self.numOfBeam,self.numOfChannel)
    #np.zeros((self.numOfBeam, self.numOfChannel))
        self.__init__()
        return self.beamChanAlloc
    
   
    
    
     #def __init__(self):
        
    #已测
    def Init_BeamLocal(self,numOfBeam):
        ## 函数功能说明
        #  完成波束位置关系的计算，即以星下点波束为中心，建立坐标系，目前实现的是以蜂窝小区为参考，进行坐标系建立，只适用于19和37小区
        #  并且以层次方式对波束小区进行编号，矩阵顺序即为从里到外，以环的层次进行编号，每环按照顺时针顺序编号
        #
        #  输入参数：小区个数
        #  输出参数：波束小区的坐标矩阵
    
    
        ## 根据不同波束个数计算相应的位置关系
        
        BeamLoca  =  np.zeros((self.numOfBeam, 2))
        # 第1层
        BeamLoca[0,0] = 0;  BeamLoca[0,1] = 0   #小区1(0,0) 
        
        
        # 第2层 6个小区
        BeamLoca[1,0] = 1;   BeamLoca[1,1] = 0;    # 小区2 [1,0]
        BeamLoca[2,0] = 1;   BeamLoca[2,1] = -1;   # 小区3 [1,-1]
        BeamLoca[3,0] = 0;   BeamLoca[3,1] = -1;   # 小区4 [0,-1]
        BeamLoca[4,0] = -1;  BeamLoca[4,1] = 0;    # 小区5 [-1,0]
        BeamLoca[5,0] = -1;  BeamLoca[5,1] = 1;    # 小区6 [-1.1]
        BeamLoca[6,0] = 0;   BeamLoca[6,1] = 1;    # 小区7 [0,1]
        
###         第3层 12个小区
#        BeamLoca[7,0]  = 2;   BeamLoca[7,1]  = 0;    # 小区8 [2,0]
#        BeamLoca[8,0]  = 2;   BeamLoca[8,1]  = -1;   # 小区9 [2,-1]
#        BeamLoca[9,0] = 2;    BeamLoca[9,1] = -2;   # 小区10 [2,-2]
#        BeamLoca[10,0] = 1;   BeamLoca[10,1] = -2;   # 小区11 [1,-2]
#        BeamLoca[11,0] = 0;   BeamLoca[11,1] = -2;   # 小区12 [0,-2]
#        BeamLoca[12,0] = -1;  BeamLoca[12,1] = -1;   # 小区13 [-1,-1]
#        BeamLoca[13,0] = -2;  BeamLoca[13,1] = 0;    # 小区14 [-2,0]
#        BeamLoca[14,0] = -2;  BeamLoca[14,1] = 1;    # 小区15 [-2,1]
#        BeamLoca[15,0] = -2;  BeamLoca[15,1] = 2;    # 小区16 [-2,2]
#        BeamLoca[16,0] = -1;  BeamLoca[16,1] = 2;    # 小区17 [-1,2]
#        BeamLoca[17,0] = 0;   BeamLoca[17,1] = 2;    # 小区18 [0,2]
#        BeamLoca[18,0] = 1;   BeamLoca[18,1] = 1;    # 小区19 [1,1]
        
        # 第4层 18个小区
#        BeamLoca[19,0] = 3;    BeamLoca[19,1] = 0;     # 小区20 [3,0]
#        BeamLoca[20,0] = 3;    BeamLoca[20,1] = -1;    # 小区21 [3,-1]
#        BeamLoca[21,0] = 3;    BeamLoca[21,1] = -2;    # 小区22 [3,-2]
#        BeamLoca[22,0] = 3;    BeamLoca[22,1] = -3;    # 小区23 [3,-3]
#        BeamLoca[23,0] = 2;    BeamLoca[23,1] = -3;    # 小区24 [2,-3]
#        BeamLoca[24,0] = 1;    BeamLoca[24,1] = -3;    # 小区25 [1,-3]
#        BeamLoca[25,0] = 0;    BeamLoca[25,1] = -3;    # 小区26 [0,-3]
#        BeamLoca[26,0] = -1;   BeamLoca[26,1] = 2;     # 小区27 [-1,-2]
#        BeamLoca[27,0] = -2;   BeamLoca[27,1] = -1;    # 小区28 [-2,-1]
#        BeamLoca[28,0] = -3;   BeamLoca[28,1] = 0;     # 小区29 [-3,0]
#        BeamLoca[29,0] = -3;   BeamLoca[29,1] = 1;     # 小区30 [-3,1]
#        BeamLoca[30,0] = -3;   BeamLoca[30,1] = 2;     # 小区31 [-3,2]
#        BeamLoca[31,0] = -3;   BeamLoca[31,1] = 3;     # 小区32 [-3,3]
#        BeamLoca[32,0] = -2;   BeamLoca[32,1] = 3;     # 小区33 [-2,3]
#        BeamLoca[33,0] = -1;   BeamLoca[33,1] = 3;     # 小区34 [-1,3]
#        BeamLoca[34,0] = 0;    BeamLoca[34,1] = 3;     # 小区35 [0,3]
#        BeamLoca[35,0] = 1;    BeamLoca[35,1] = 2;     # 小区36 [1,2]
#        BeamLoca[36,0] = 2;    BeamLoca[36,1] = 1;     # 小区37 [2,1]
#        
        
        ## 获取相应的波束位置关系矩阵
        matrixOfBeamLoca  =  BeamLoca[0:numOfBeam,:]
        
        return matrixOfBeamLoca
    
     #已测
    def Cal_DistanceOfBeam(self,
                           r,
                           matrixOfBeamLoca,
                           ):
        
        ## 函数功能说明
        #  根据波束之间的坐标关系，计算波束小区中心之间的距离  
        #  计算公式为D = sqrt(3) * R * sqrt( (x1-x2)^2 + (y1-y2)^2 + (x1-x2)*(y1-y2))
        #  其中R为小区半径，(x1,y1),(x2,y2)分别为不同小区的中心坐标
        #
        #  输入参数：
        #      小区半径R;  
        #      波束坐标矩阵matrixOfBeamLoca
        #
        #  输出参数：
        #      波束小区之间的距离关系矩阵matrixOfBeamDis
        
        ## 计算所有波束小区中心之间的距离
        
        row, col = matrixOfBeamLoca.shape           # 获取波束小区的个数

        matrixOfBeamDis  =  np.zeros((row, row))    # 初始化波束小区距离关系矩阵
        
        for i in range(row):
           for j in range(row): 
               x1 = matrixOfBeamLoca[i,0];
               y1 = matrixOfBeamLoca[i,1];
               x2 = matrixOfBeamLoca[j,0];
               y2 = matrixOfBeamLoca[j,1];
               matrixOfBeamDis[i, j] = self.Cal_Distance(x1, x2, y1, y2, r);
               
        return matrixOfBeamDis
     
         #已测
    def Cal_Distance(self,x1, x2, y1, y2, r):
        
        distance = np.sqrt(3) * r * np.sqrt( (x1-x2)**2 + (y1-y2)**2 + (x1-x2)*(y1-y2))
        
        return distance
    
    
    
    #已测
    def Fun_UpdateChanAlloc(self,matrixOfChanAlloc, matrixOfChanOfUseTime):

        # 更新信道占用时间中大于1的元素
        matrixOfChanOfUseTime[np.where(matrixOfChanOfUseTime > 0)]  =  matrixOfChanOfUseTime[np.where(matrixOfChanOfUseTime > 0)] - 1
        
        matrixOfChanAlloc[np.where(matrixOfChanOfUseTime == 0)]     =  0
        matrixOfChanAlloc[np.where(matrixOfChanOfUseTime > 0)]      =  1
        
    
    
        return matrixOfChanAlloc, matrixOfChanOfUseTime
    
   
       
     
       
    
    
    
    ## 计算可用波束可用信道矩阵
    #已测
    def Cal_BeamValidChan(self,beamChanAlloc, beamOfDistance):
        
        #最小复用距离
        minDist  = 2
        # 取出占用该信道的波束和信道编号索引
        indexRow, indexCol =  np.where(beamChanAlloc == 1) 
        
        beamValidChan         =  beamChanAlloc
        for i in range(indexRow.size):
             #小区一定距离范围内不能占用同一个频率信道,取出小于这个距离的小区索引 
           indexBeamOfInter  =  np.where(beamOfDistance[indexRow[i],:] < minDist)   
           beamValidChan[indexBeamOfInter,indexCol[i]] = -1
    
        return beamValidChan
    #已测
#    def Init_Environment(self,beamChanAlloc, beamOfUser, beamOfDistance):
#        # 建立信道分配矩阵的初始状态
#        
##        Sat_Env.beamUser       =  beamOfUser
##        Sat_Env.User           =  beamOfUser
##        Sat_Env.beamChanAlloc  =  beamChanAlloc         
#        
#        
#        # 初始化波束可用矩阵
#        initEnvState       =  Sat_Env.Cal_BeamValidChan(Sat_Env.beamChanAlloc, beamOfDistance)   
#         
#        return initEnvState
    #已测
        ## 计算阻塞用户数
    def Cal_Block(beamOfUser, beamChanAlloc):
        blockNum = 0
        row  =  beamOfUser.size
        for i in range(row):
            if sum(beamChanAlloc[i,:]) < beamOfUser[i]:
                blockNum  =  blockNum + (beamOfUser[i] - sum(beamChanAlloc[i,:]))
            
            return blockNum
    #已测
        ## 计算阻塞用户数
#    def Cal_Block(beamOfUser, beamChanAlloc):
#        blockNum = 0
#        row  =  beamOfUser.size
#        for i in range(row):
#            if sum(beamChanAlloc[i,:]) < beamOfUser[i]:
#                blockNum  =  blockNum + (beamOfUser[i] - sum(beamChanAlloc[i,:]))
#            
#            return blockNum
