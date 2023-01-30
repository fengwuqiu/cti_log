#include <stdio.h>
#include "ctilog/log.hpp"
#include "ctilog/loghelper.cpp.hpp"
#include "ros/ros.h"
#include <std_msgs/Int32.h>

//KN:name in log
constexpr char const* kN = "main";
using namespace cti::log;

void logLevelCallback(const std_msgs::Int32 &msg)
{
    LogLevel lev = (LogLevel)msg.data;
    Logger::getLogger().setLogLevel(lev);
    Info("rev log level " << logLevelToString(lev));
}

int main(int argv, char *argc[])
{
  ros::init(argv, argc, "cti_log_example");
  
  ros::NodeHandle n;

  //set the logger file name, defaut is "logger.log"
  Logger::setDefaultLogger("/home/lrd/test.log");

  //set log output mode {CoutOrCerr,File,Both}
  Logger::getLogger().setOutputs(Logger::Output::Both);

  //set log level mode {Fata,Erro,Warn,Note,Info,Trac,Debu,Deta}
  Logger::getLogger().setLogLevel(LogLevel::Debu);

  //enable display thread id , defaut is "false"
  Logger::getLogger().enableTid(false);

  //enable display line id number, defaut is "true"
  Logger::getLogger().enableIdx(true);

  //默认最小8*1024 默认最大256*1024*1024  256M
  Logger::getLogger().setMaxSize(1024*1024);

  ros::Subscriber sub = n.subscribe("/cti/log/level",2,logLevelCallback);
  
  while(ros::ok()){
    //logger
    Fatal("test-Fata!");
    Error("test-Erro!");
    Warn("test-Warn!");
    Note("test-Note!");
    Info("test-Info!");
    Trace();
    Debug("test-Debu!");
    Detail("test-Deta!");
    //--
    try {
      Assert(12==3,"you are wrong!");
    }
    catch(std::exception const& e) {
      Error("error:"<<e.what());
    }
    try{
        Throw("some things error!");
    }catch(std::exception const& e) {
        Error("error:"<<e.what());
    }

    //--显示数据
    //打印数据
    int data1=100;
    float data2 = 102.25;
    std::string data3 = "this is ctilog test!";
    Info("data1="<<data1<<" data2="<<data2<<" data3="<<data3);
  
    for(int i=0;i<10;i++){
        ros::Duration(0.5).sleep();
        ros::spinOnce();
    }
  }
  return 0;
}
