#!/usr/bin/python
# -*- coding: utf-8 -*-

import tensorflow as tf
import numpy as np
from tensorflow.python.framework import ops
# tf.reset_default_graph()
# import tensorflow.contrib.keras as kr
class TRNNConfig(object):
    """RNN配置参数"""

    # 模型参数
    num_instance = 1
    num_step = 1200                               # 时间步长/展开的rnn的block个数
    num_classes = 12                            # 类别数====用户所在波束下的所有信道数

    # seq_length_batch = [64 for i in range(shape[0])]
    # print(seq_length_batch)                 # 序列长度====所有波束下的所有信道（最大也不可能全部占用）
    num_layers = 2                              # 隐藏层层数
    hidden_dim = 128                            # 隐藏层神经元
    rnn = 'gru'                                 # lstm 或 gru

    dropout_keep_prob = 0.3                     # dropout保留比例
    learning_rate = 1e-3                        # 学习率
    gamma = 0.95

    batch_size = 128                            # 每批训练大小
    num_epochs = 10                             # 总迭代轮次

    print_per_batch = 100                       # 每多少轮输出一次结果
    save_per_batch = 10                         # 每多少轮存入tensorboard
    load_path = None,
    save_path = None


class ResourceAllocationRNN(object):
    """资源分配，RNN模型"""
    def __init__(self, config):
        self.config = config

        # 三个待输入的数据
        self.input_x = tf.placeholder(tf.float32, [None,  self.config.num_step, 4], name='input_x')
        shape = self.input_x.shape.as_list()
        # print(shape[0])
        self.seq_length = shape[0]
        self.input_y = tf.placeholder(tf.float32, [None, self.config.num_step, self.config.num_classes], name='input_y')
        self.discounted_episode_rewards_norm = tf.placeholder(tf.float32, [None, self.config.num_step], name="actions_value")
        self.keep_prob = tf.placeholder(tf.float32, name='keep_prob')

        self.episode_observations, self.episode_actions, self.episode_rewards, self.episode_single_step_actions = [], [], [], []
        self.sess = tf.Session()
        self.sess.run((tf.global_variables_initializer(), tf.local_variables_initializer()))
        self.loss = None
        self.rnn()
        self.save_path = None
        load_path = None,
        save_path = None
        if save_path is not None:
            self.save_path = save_path



    def rnn(self):

        """rnn模型"""

        def lstm_cell():   # lstm核
            return tf.contrib.rnn.BasicLSTMCell(self.config.hidden_dim, state_is_tuple=True)

        def gru_cell():  # gru核
            return tf.contrib.rnn.GRUCell(self.config.hidden_dim)

        def dropout():  # 为每一个rnn核后面加一个dropout层
            if (self.config.rnn == 'lstm'):
                cell = lstm_cell()
            else:
                cell = gru_cell()
            return tf.contrib.rnn.DropoutWrapper(cell, output_keep_prob=self.keep_prob)

        # 词向量映射
        # with tf.device('/cpu:0'):
        #     embedding = tf.get_variable('embedding', [self.config.vocab_size, self.config.embedding_dim])
        #     embedding_inputs = tf.nn.embedding_lookup(embedding, self.input_x)
        #     print("---------------------------1")
        with tf.name_scope("rnn"):
            # 多层rnn网络
            cells = [dropout() for _ in range(self.config.num_layers)]
            rnn_cell = tf.contrib.rnn.MultiRNNCell(cells, state_is_tuple=True)

            _outputs, _ = tf.nn.dynamic_rnn(cell=rnn_cell, inputs=self.input_x, dtype=tf.float32, sequence_length=self.seq_length)  #_outputs
            # print("_outputs", _outputs)
            #last = _outputs[:, -1, :]  # 取最后一个时序输出作为结果
            # print("---------------------------2")
        with tf.name_scope("score"):
            # 全连接层，后面接dropout以及relu激活
            fc = tf.layers.dense(_outputs, self.config.hidden_dim, name='fc1')
            fc = tf.contrib.layers.dropout(fc, self.keep_prob)
            fc = tf.nn.relu(fc)
            # print("---------------------------3")
            # 分类器
            self.logits = tf.layers.dense(fc, self.config.num_classes, name='fc2')
            # self.logits = tf.unstack(self.logits, axis=0)
            self.y_pred_cls = tf.argmax(tf.nn.softmax(self.logits), 2) # 预测类别

        with tf.name_scope("optimize"):
            # 损失函数，交叉熵
            cross_entropy = tf.nn.softmax_cross_entropy_with_logits(logits=self.logits, labels=self.input_y)
            self.loss = tf.reduce_mean(cross_entropy * self.discounted_episode_rewards_norm)

            # print("---loss:",self.loss)
            # 优化器
            self.optim = tf.train.AdamOptimizer(learning_rate=self.config.learning_rate).minimize(self.loss)
            # print("---------------------------4")
        # with tf.name_scope("accuracy"):
        #     print("---------------------------5")
        #     #准确率
        #     correct_pred = tf.equal(tf.argmax(self.input_y, 1),self.y_pred_cls)
        #     self.acc = tf.reduce_mean(tf.cast(correct_pred, tf.float32))

    def choose_action(self, observation_step):
        """
            Choose action based on observation
        """
        action = np.zeros([7, 12])
        self.sess = tf.Session()
        self.sess.run((tf.global_variables_initializer(), tf.local_variables_initializer()))
        channel_of_user = self.sess.run(self.y_pred_cls, feed_dict={self.input_x: observation_step, self.keep_prob: 0.3})                         # channel_of_user表示所有用户经rnn网络选择的信道 （1*84维的向量）
        # channel_of_user = self.sess.run(self.y_pred_cls, feed_dict = {self.input_x: kr.preprocessing.sequence.pad_sequences(np.vstack(observation).T,self.seq_length),self.keep_prob:1.0})
        # print("channel_of_user:", channel_of_user)

        for instance in range(self.config.num_instance):
            for index_of_user in range(1200):
                RNTI_of_user = observation_step[instance][index_of_user][1]
                if RNTI_of_user == 0:
                    continue
                beam_of_user = observation_step[instance][index_of_user][0]
                if action[beam_of_user][channel_of_user[instance][index_of_user]] != 0:
                    continue
                #print(beam_of_user)      # beam_of_user表示该用户所要接入的波束
                action[beam_of_user][channel_of_user[instance][index_of_user]] = RNTI_of_user    # 将用户的RNTI填充至信道占用矩阵的相应位置
            # print("action", action)
        return action


        

    def choose_action2(self, observation_step):
        """
            Choose action based on observation
        """
        action = np.zeros([7, 12])
        tj = np.zeros([7, 12])
        self.sess = tf.Session()
        self.sess.run((tf.global_variables_initializer(), tf.local_variables_initializer()))
        channel_of_user = self.sess.run(self.y_pred_cls, feed_dict={self.input_x: observation_step, self.keep_prob: 0.3})                         # channel_of_user表示所有用户经rnn网络选择的信道 （1*84维的向量）
        # channel_of_user = self.sess.run(self.y_pred_cls, feed_dict = {self.input_x: kr.preprocessing.sequence.pad_sequences(np.vstack(observation).T,self.seq_length),self.keep_prob:1.0})
        # print("channel_of_user:", channel_of_user)

        for instance in range(self.config.num_instance):
            for index_of_user in range(1200):
                RNTI_of_user = observation_step[instance][index_of_user][1]
                if RNTI_of_user == 0:
                    continue
                beam_of_user = observation_step[instance][index_of_user][0]
                # if action[beam_of_user][channel_of_user[instance][index_of_user]] != 0:
                #     continue
                #print(beam_of_user)      # beam_of_user表示该用户所要接入的波束
                action[beam_of_user][channel_of_user[instance][index_of_user]] = RNTI_of_user    # 将用户的RNTI填充至信道占用矩阵的相应位置
                tj[beam_of_user][channel_of_user[instance][index_of_user]] = tj[beam_of_user][channel_of_user[instance][index_of_user]] + 1
            # print("action", action)
        return action,tj


    def store_transition(self, s, a, r):
        """
            Store play memory for training

            Arguments:
                s: observation
                a: action taken
                r: reward after action
        """
        # 将状态s，动作a， 奖赏r 分别存到一个列表中。
        self.episode_observations.append(s)    # 状态存入状态序列
        self.episode_rewards.append(r)         # 奖赏存入奖赏序列
        self.episode_actions.append(a)         # 动作存入动作序列 （此处的动作是一个TTI下的总动作）

        #除此之外，还要存一个单个用户的动作的列表，为了学习。
        # Store actions as list of arrays
        # e.g. for input_y = 2 -> [ array([ 1.,  0.]), array([ 0.,  1.]), array([ 0.,  1.]), array([ 1.,  0.]) ]
        single_step_action = np.zeros([self.config.num_instance, self.config.num_step, self.config.num_classes])
        channel_of_user = self.sess.run(self.y_pred_cls, feed_dict={self.input_x: s, self.keep_prob: 0.3})

        for instance in range(self.config.num_instance):
            for line in range(self.config.num_step):
                vertical = channel_of_user[instance][line]
                single_step_action[instance][line][vertical] = 1
        # print("single_step_action", single_step_action)
        self.episode_single_step_actions.append(single_step_action)
        print("---episode_rewards: ",self.episode_rewards)
        return self.episode_single_step_actions, self.episode_observations, self.episode_rewards

    def learn(self):
        # Discount and normalize episode reward
        discounted_episode_rewards_norm = self.discount_and_norm_rewards()

        # Train on episode
        print("----rewards:", discounted_episode_rewards_norm)
        self.sess.run(self.optim, feed_dict={
            self.input_x: np.vstack(self.episode_observations[0:len(self.episode_observations)-3]),
            self.input_y: np.vstack(self.episode_single_step_actions[0:len(self.episode_single_step_actions)-3]),
            self.discounted_episode_rewards_norm: discounted_episode_rewards_norm,
            self.keep_prob: 0.3
        })
        print("----loss: ",self.sess.run(self.loss, feed_dict={
            self.input_x: np.vstack(self.episode_observations[0:len(self.episode_observations)-3]),
            self.input_y: np.vstack(self.episode_single_step_actions[0:len(self.episode_single_step_actions)-3]),
            self.discounted_episode_rewards_norm: discounted_episode_rewards_norm,
            self.keep_prob: 0.3
        }))
        # Reset the episode data
        self.episode_observations = self.episode_observations[len(self.episode_observations)-3:]
        self.episode_actions = self.episode_actions[len(self.episode_actions)-3:]
        self.episode_single_step_actions = self.episode_single_step_actions[len(self.episode_single_step_actions)-3:]
        self.episode_rewards = self.episode_rewards[0:3]
        # self.episode_single_step_actions,self.episode_observations, self.episode_actions, self.episode_rewards = [], [], [], []
        
        # Save checkpoint
        if self.save_path is not None:
            save_path = self.saver.save(self.sess, self.save_path)
            # print("Model saved in file: %s" % save_path)
        print("---loss:",self.loss)
        

        return discounted_episode_rewards_norm

    def discount_and_norm_rewards(self):

        discounted_episode_rewards = np.zeros([len(self.episode_rewards)-3, self.config.num_step])
        # print("shape_of_discounted_episode_rewards", discounted_episode_rewards.shape)
        cumulative = 0
        for t in reversed(range(len(self.episode_rewards[3:]))):
            cumulative = cumulative * self.config.gamma + self.episode_rewards[t]
            discounted_episode_rewards[t][1199] = cumulative
            discounted_episode_rewards -= np.mean(discounted_episode_rewards)
            discounted_episode_rewards /= np.std(discounted_episode_rewards)

        return discounted_episode_rewards

    def plot_cost(self):
        import matplotlib
        matplotlib.use("MacOSX")
        import matplotlib.pyplot as plt
        plt.plot(np.arange(len(self.cost_history)), self.cost_history)
        plt.ylabel('Cost')
        plt.xlabel('Training Steps')
        plt.show()