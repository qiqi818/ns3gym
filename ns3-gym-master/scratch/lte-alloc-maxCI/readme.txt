--基于Max C/I算法的调度器性能仿真
--运行结果输出存储在本目录下的throughoutput.txt（吞吐量）和throughoutputrate.txt（吞吐率）
1、由于CqaFfMacScheduler调度器没有改写，任然沿用了重构前的框架
2、由于需要Getreward函数和GetObservation函数统计信息，仍然需要同时运行C++和python脚本。
3、C++中的GetObservation函数的返回值返回空值，python侧不再进行学习和决策
4、MyExecuteActions函数中的执行动作代码注释
