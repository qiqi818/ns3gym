#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import gym

import numpy as np
# from rnn import ResourceAllocationRNN
import random
import argparse
from ns3gym import ns3env
from gym import spaces
import random
from policy_gradient import PolicyGradient

# from rnn import TRNNConfig, ResourceAllocationRNN
# import tensorflow as tf
import matplotlib.pyplot as plt
# __author__ = "Piotr Gawlowicz"
# __copyright__ = "Copyright (c) 2018, Technische Universität Berlin"
# __version__ = "0.1.0"
# __email__ = "gawlowicz@tkn.tu-berlin.de"

parser = argparse.ArgumentParser(description='Start simulation script on/off')
parser.add_argument('--start',
                    type=int,
                    default=1,
                    help='Start ns-3 simulation script 0/1, Default: 1')
parser.add_argument('--iterations',
                    type=int,
                    default=1,
                    help='Number of iterations, Default: 1')
args = parser.parse_args()
startSim = bool(args.start)
iterationNum = int(args.iterations)

port = 5555
simTime = 200  # seconds
stepTime = 0.5  # seconds
seed = 0
simArgs = {"--simTime": simTime,
           "--testArg": 123}
debug = False

# ------------------------------------------------------------------------------------------------

if __name__ == "__main__":
    # Load checkpoint
    load_path = None
    save_path = None
    PG = PolicyGradient(n_x = 252,n_y = 4,learning_rate=0.05,reward_decay=0.95,load_path=load_path,save_path=save_path)
    #创建RNN网络
    # config = TRNNConfig()
    # RNN = ResourceAllocationRNN(config)
#/home/liqi/.local/lib/python3.6/site-packages/ns3gym
#初始化环境env
env = ns3env.Ns3Env(port=port, stepTime=stepTime, startSim=startSim, simSeed=seed, simArgs=simArgs, debug=debug)
env.reset()

ob_space = env.observation_space
ac_space = env.action_space
ax = []
ay = []
az = []
stepIdx = -1
currIt = 0
plt.ion()

	
try:
    while True:
        print("Start iteration: ", currIt)
        obs = env.reset()
        # print("Step: ", stepIdx)
        # print("---obs:", obs)

        while True:
            stepIdx += 1
            ax.append(stepIdx) 
            # ---------------------------------------------------------------------------------------
            print(obs)
            observation = []#环境的观测值，状态observation
            for i in range(len(obs)):
                observation.append(obs[i])
            while len(observation) < 252:
                observation.append(0)
            observation_step = np.array(observation).copy().reshape(252,1).ravel()





            # observation_step = observation 
            # 将状态输入RNN选取动作  
            if(stepIdx < 100):
                action_step = random.randint(0,3)
            else:
                action_step = PG.choose_action(observation_step)
            az.append(action_step)
            # action_step = 1
            # action_list = []
            # #将动作以[CellId1,RNTI1,资源编号1,CellId2,RNTI2,资源编号2,……]的形式保存在action_list中
            # for x in range(84):
            #     action_list.append(observation[x][0])
            #     action_list.append(observation[x][1])
            #     action_list.append(random.randint(0,11))
            #     # for y in range(12):
            #     #     if action_step[x][y] > 0:
            #     #         action_list.append(x)                       #CellId
            #     #         action_list.append(int(action_step[x][y]))  #RNTI
            #     #         action_list.append(y)                       #资源编号
            # #将动作封装到spaces.Tuple类型中，以备传送给ns3
            # # print(action_list)
            # d = (spaces.Discrete(0),spaces.Discrete(0),spaces.Discrete(0))
            # for b in range(len(action_list)):
            #     d += (spaces.Discrete(int(action_list[b])),)
            d = (spaces.Discrete(action_step),)
            action_ = spaces.Tuple(d)
            print("action_step:",action_step)
            print("Step: ", stepIdx)
            print("---obs:\n ", obs)
            # print("---action:\n ", action_list)
            
            #step函数，将动作action_传给ns3中执行下去，并获得ns3中的反馈
            obs, reward_step, done, info = env.step(action_)
            
            ay.append(reward_step)
            print("---reward: ", reward_step)
            print("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-")
            #将每一步的状态、动作、奖励存储，以备进行学习
            s, a, r = PG.store_transition(observation_step, action_step, reward_step)

            # plt.clf()              # 清除之前画的图
            # plt.plot(ax,ay,'b')        # 画出当前 ax 列表和 ay 列表中的值的图形
            # plt.plot(ax,az,'r',linestyle='--',marker='o')
            # plt.xlabel('step')
            # plt.ylabel('throughout')
            # plt.pause(0.01)         # 暂停一秒
            # plt.ioff()             # 关闭画图的窗口
            
            # #每经历10个step，网络进行一次学习
            if  stepIdx > 0 and stepIdx%30 == 0:
                
                discounted_episode_rewards_norm = PG.learn()


            if done:
                stepIdx = 0
                if currIt + 1 < iterationNum:
                    env.reset()
                break

        currIt += 1
        if currIt == iterationNum:
            break

except KeyboardInterrupt:
    print("Ctrl-C -> Exit")
finally:
    # env.close()
    print("Done")


