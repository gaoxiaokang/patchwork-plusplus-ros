#include <iostream>
// For disable PCL complile lib, to use PointXYZIR
#define PCL_NO_PRECOMPILE
#include <jsk_recognition_msgs/ground_estimate.h>
#include <ros/ros.h>
#include <signal.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl_conversions/pcl_conversions.h>

#include "patchworkpp/patchworkpp.hpp"

using PointType = pcl::PointXYZI;
using namespace std;

boost::shared_ptr<PatchWorkpp<PointType>> PatchworkppGroundSeg;

ros::Publisher pub_cloud;
ros::Publisher pub_ground;
ros::Publisher pub_non_ground;
ros::Publisher EstimatePublisher;

template<typename T>
sensor_msgs::PointCloud2 cloud2msg(pcl::PointCloud<T> cloud, std::string frame_id = "map") {
    sensor_msgs::PointCloud2 cloud_ROS;
    pcl::toROSMsg(cloud, cloud_ROS);
    cloud_ROS.header.frame_id = frame_id;
    return cloud_ROS;
}

void callbackCloud(const sensor_msgs::PointCloud2::Ptr &cloud_msg)
{
    double time_taken;

    pcl::PointCloud<PointType> pc_curr;
    pcl::PointCloud<PointType> pc_ground;
    pcl::PointCloud<PointType> pc_non_ground;
    
    pcl::fromROSMsg(*cloud_msg, pc_curr);

    PatchworkppGroundSeg->estimate_ground(pc_curr, pc_ground, pc_non_ground, time_taken);

    cout << "\033[1;32m" << "Result: Input PointCloud: " << pc_curr.size() << " -> Ground: " << pc_ground.size() <<  "/ NonGround: " << pc_non_ground.size()
         << " (running_time: " << time_taken << " sec)" << "\033[0m" << endl;
    
    //pcl->rosmsg
    auto msg_curr = cloud2msg(pc_curr);
    auto msg_ground = cloud2msg(pc_ground);
    //cloud_estimate
    jsk_recognition_msgs::ground_estimate cloud_estimate;
    cloud_estimate.header = cloud_msg->header;
    cloud_estimate.curr = msg_curr;
    cloud_estimate.ground = msg_ground;


    //cloud_estimate包含curr与ground
    EstimatePublisher.publish(cloud_estimate);

    pub_cloud.publish(cloud2msg(pc_curr));
    pub_ground.publish(cloud2msg(pc_ground));
    pub_non_ground.publish(cloud2msg(pc_non_ground));
}

int main(int argc, char**argv) {

    ros::init(argc, argv, "patchworkpp");
    ros::NodeHandle nh;

    std::string cloud_topic;
    nh.param<string>("/cloud_topic", cloud_topic, "/pointcloud");

    cout << "Operating patchwork++..." << endl;
    PatchworkppGroundSeg.reset(new PatchWorkpp<PointType>(&nh));

    pub_cloud       = nh.advertise<sensor_msgs::PointCloud2>("/patchworkpp/cloud", 100, true);
    pub_ground      = nh.advertise<sensor_msgs::PointCloud2>("/patchworkpp/ground", 100, true);
    pub_non_ground  = nh.advertise<sensor_msgs::PointCloud2>("/patchworkpp/nonground", 100, true);
    
    /* Publisher for combined msg of source cloud, ground cloud */
    EstimatePublisher = nh.advertise<jsk_recognition_msgs::ground_estimate>("/benchmark/ground_estimate", 100, true);
    
    ros::Subscriber sub_cloud = nh.subscribe(cloud_topic, 100, callbackCloud);


    while (ros::ok())
    {
        ros::spinOnce();
    }
    
    return 0;
}
