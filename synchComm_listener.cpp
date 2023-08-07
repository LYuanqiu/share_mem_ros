#include "ros/ros.h"
//#include "message/shareMem.h"
#include <sstream>
#include "shareMem.h"
int main(int argc, char **argv)
{
    ros::init(argc, argv, "talker");
    ros::NodeHandle n;

    ros::Rate loop_rate(10);

    synchredShm read("synchComm_talker");

    int count = 0;
    while (++count < 20)
    {

        std_msgs::String msg;
        char *rinfo;
        int rsize = read.rece(rinfo);
        std::stringstream ss;
        ss << rinfo;
        msg.data = ss.str();

        ROS_INFO("%s", msg.data.c_str());

        char *info = "I AM RESPONDING!!!";
        read.send(info, 19); // todo 信息大小还需要改变

        ss.clear();
        ss << info; // todo 不知道对不对
        msg.data = ss.str();

        ROS_INFO("%s", msg.data.c_str());

        ros::spinOnce();

        loop_rate.sleep();
        ++count;
    }
    read.releaseMem();
    return 0;
}
