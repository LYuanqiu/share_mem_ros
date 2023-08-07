#include "ros/ros.h"
//#include "message/shareMem.h"
#include <sstream>
#include "shareMem.h"
int main(int argc, char **argv)
{
    ros::init(argc, argv, "talker");
    ros::NodeHandle n;

    ros::Rate loop_rate(10);

    synchwriShm write("synchComm_talker");

    int count = 0;
    while (++count < 20)
    {

        std_msgs::String msg;

        char *info = "I AM TALKING!!!";
        char *rinfo;
        write.send(info, 16, rinfo); // todo 信息大小还需要改变

        std::stringstream ss;
        ss << rinfo; // todo 不知道对不对
        msg.data = ss.str();

        ROS_INFO("%s", msg.data.c_str());

        ros::spinOnce();

        loop_rate.sleep();
        ++count;
    }
    write.releaseMem();
    return 0;
}
