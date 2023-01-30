#include <stdio.h>
#include <rclcpp/rclcpp.hpp>
#include <ctilog/log.hpp>
#include <ctilog/loghelper.cpp.hpp>
#include <std_msgs/msg/int32.hpp>

//KN:name in log
constexpr char const* kN = "main";
using namespace cti::log;

void logLevelCallback(const std_msgs::msg::Int32::SharedPtr msg)
{
    LogLevel lev = (LogLevel)msg->data;
    Logger::getLogger().setLogLevel(lev);
    Info("rev log level " << logLevelToString(lev));
}

int main(int argv, char *argc[])
{
  rclcpp::init(argv, argc);
  
  //ros::NodeHandle n;
  auto node = rclcpp::Node::make_shared("cti_log_example");

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

  //ros::Subscriber sub = node->subscribe("/cti/log/level",2,logLevelCallback);
  auto sub = node->
  create_subscription<std_msgs::msg::Int32>("/cti/log/level",10,logLevelCallback);
  
  while(rclcpp::ok()){
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
        //rclcpp::Duration(0.5).sleep_for();
        rclcpp::spin(node);
    }
  }
  return 0;
}
