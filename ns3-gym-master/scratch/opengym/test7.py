#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import gym
# import matplotlib.pyplot as plt
import numpy as np
from rnn import ResourceAllocationRNN
import random
import argparse
from ns3gym import ns3env
from gym import spaces
from rnn import TRNNConfig, ResourceAllocationRNN
import tensorflow as tf
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
# /home/liqi/.local/lib/python3.6/site-packages/gym/envs/
env1 = gym.make('sateDCA_ENV-v0')
# env1 = env1.unwrapped

if __name__ == "__main__":
    # Load checkpoint
    load_path = None
    save_path = None

    config = TRNNConfig()
    # RNN = ResourceAllocationRNN(config)

env = ns3env.Ns3Env(port=port, stepTime=stepTime, startSim=startSim, simSeed=seed, simArgs=simArgs, debug=debug)

env.reset()

# p = tf.placeholder("float", shape=(None,3))
# s = tf.multinomial(p,10)
# sess = tf.Session()
# # y = sess.run(tf.global_variables_initializer())
# y = sess.run(s, feed_dict={p:[[0.2,0.3,0.5],[0.7,0.2,0.1],[0.7,0.2,0.1]]})
# print(y)
# print(y[0][0])
# sess.close()

ob_space = env.observation_space
ac_space = env.action_space

stepIdx = 0
currIt = 0

try:
    while True:
        print("Start iteration: ", currIt)
        obs = env.reset()
        print("Step: ", stepIdx)
        print("---obs:", obs)

        while True:
            stepIdx += 1

            # ---------------------------------------------------------------------------------------
            rewards = []
            EPISODES = 1
            # env1.reset()
            # for i in range(EPISODES):  # -------------------------------------------------------学习片段
                # matrixOfChanAlloc = np.zeros((7, 12))  # 信道占用矩阵
                # channel_alloc = matrixOfChanAlloc.copy()
                # env1.beamChanAlloc = channel_alloc.copy()

                # env1.beamOfUser = np.array([1, 1, 1, 1, 1, 1, 1])  # obs 

                # for j in range(7):
                #     env1.beamOfUser[j] = (int)(obs[j])  # 观测状态 每个波束下的用户数

                # sumu = sum(env1.beamOfUser)
                # if (env1.beamOfUser == 0).all():
                #     continue
                # observation = env1.beamChanAlloc
            episode_reward = 0
            if obs is None:
                obs = [0]*(2*84)
                # continue
            # obs[0] = 6
            # obs[1] = 4
            observation = []
            if len(obs)<168:
                for kk in range(168 - len(obs)):
                    obs.append(0)
            for j in range(84):
                observation.append([obs[2*j],obs[2*j+1]])
            print(observation) 
            observation = [observation]   
            print(observation) 
            # while True:

                # 1. Choose an action based on observation

                #observation = observation.copy().reshape(7 * 12, 1).ravel()
                # observation 从环境中取
                # action = np.zeros([7, 12])
                # if (obs is None) or (obs!=0)[0].shape[0]==0:
                #     action = np.zeros([7, 12])
                # else:
                # action = RNN.choose_action(observation)
                # print(action)
            # if flag:
                # episode_rewards_sum = sum(RNN.episode_rewards)
                # rewards.append(episode_rewards_sum)
                # max_reward_so_far = np.amax(rewards)
                # s, a, r = RNN.store_transition(observation, action, reward)
                # # 4. Train neural network
                # discounted_episode_rewards_norm = RNN.learn()
                
                # break

                # 2. Take action in the environment
                # observation_, reward, done, info = env1.step(action)

                # 3. Store transition for training
                

                # if done and (not flag):
                #     episode_rewards_sum = sum(RNN.episode_rewards)
                #     rewards.append(episode_rewards_sum)
                #     max_reward_so_far = np.amax(rewards)

                #     # 4. Train neural network
                #     discounted_episode_rewards_norm = RNN.learn()

                #     # Render env if we get to rewards minimum

                #     break

                # Save new observation
                # observation = observation_

            # if (obs is None) or (obs!=0)[0].shape[0]==0:
            #     action = np.zeros([7, 12])

            action=[2,0,1]
        # if (obs!=0)[0].shape[0]==0:
            action_list = [9999]
            d = (spaces.Discrete(5),spaces.Discrete(14),spaces.Discrete(1))
            action_ = spaces.Tuple(d)
        # else:
        #     d = ()
        #     action_list = []
        #     for i in range(7):
        #         for j in range(12):
        #             if(action > 0):
        #                 d += (spaces.Discrete(i),spaces.Discrete(action[i][j]),spaces.Discrete(j))
        #     action_ = spaces.Tuple(d)

            # -----------------------------------------------------------------------------

            # ---------------------------------------------------------------------------------------
            
            print("---action: ", action_)
            print("Step: ", stepIdx)
            obs, reward, done, info = env.step(action_)
            print("---obs: ", obs)
            print("---reward: ", reward)
            # episode_rewards_sum = sum(RNN.episode_rewards)
            # rewards.append(episode_rewards_sum)
            # max_reward_so_far = np.amax(rewards)
            
            # 4. Train neural network
            
            # s, a, r = RNN.store_transition(observation, action, reward)
            # discounted_episode_rewards_norm = RNN.learn()
            # print("---obs, reward, done, info: ", obs, reward, done, info)
                #learn



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

