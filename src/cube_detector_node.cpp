#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <limits>


namespace cube_detector{
    class CubeDetector : public rclcpp::Node{
    private:
        rclcpp::Publisher<geometry_msgs::msg::Point>::SharedPtr pub_red_;
        rclcpp::Publisher<geometry_msgs::msg::Point>::SharedPtr pub_green_;
        rclcpp::Publisher<geometry_msgs::msg::Point>::SharedPtr pub_blue_;
        rclcpp::Publisher<geometry_msgs::msg::Point>::SharedPtr pub_white_;
        rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_image_;
        rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_image_;

        enum class CubeColor{
            RED,
            GREEN,
            BLUE,
            WHITE
        };

        void image_callback(const sensor_msgs::msg::Image::ConstSharedPtr& msg){
            try{
                cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, "bgr8");
                cv::Mat& frame = cv_ptr->image;
                cv::Mat hsv_frame;
                cv::cvtColor(frame, hsv_frame, cv::COLOR_BGR2HSV);
            
                process_and_publish(hsv_frame, frame, CubeColor::RED, pub_red_);
                process_and_publish(hsv_frame, frame, CubeColor::GREEN, pub_green_);
                process_and_publish(hsv_frame, frame, CubeColor::BLUE, pub_blue_);
                process_and_publish(hsv_frame, frame, CubeColor::WHITE, pub_white_);

                pub_image_->publish(*cv_ptr->toImageMsg());
            }
            catch (cv_bridge::Exception& e){
                RCLCPP_ERROR(this->get_logger(), "CV_BRIDGE ERROR: %s", e.what());
            }
        }

        void process_and_publish(const cv::Mat& hsv_frame, cv::Mat& draw_frame, const CubeColor& color, const rclcpp::Publisher<geometry_msgs::msg::Point>::SharedPtr& publisher) {
            cv::Mat mask;
            cv::Scalar bbox_color;

            switch(color){
                case CubeColor::RED: {
                    cv::Mat mask1, mask2;
                    int s_min = this->get_parameter("red.s_min").as_int();
                    int s_max = this->get_parameter("red.s_max").as_int();
                    int v_min = this->get_parameter("red.v_min").as_int();
                    int v_max = this->get_parameter("red.v_max").as_int();

                    cv::inRange(hsv_frame, 
                        cv::Scalar(this->get_parameter("red.h_min1").as_int(), s_min, v_min), 
                        cv::Scalar(this->get_parameter("red.h_max1").as_int(), s_max, v_max), mask1);
                    cv::inRange(hsv_frame, 
                        cv::Scalar(this->get_parameter("red.h_min2").as_int(), s_min, v_min), 
                        cv::Scalar(this->get_parameter("red.h_max2").as_int(), s_max, v_max), mask2);
                    cv::bitwise_or(mask1, mask2, mask);
                    bbox_color = cv::Scalar(0, 0, 255);
                    break;
                }
                case CubeColor::GREEN: {
                    cv::inRange(hsv_frame, get_lower_bound("green"), get_upper_bound("green"), mask);
                    bbox_color = cv::Scalar(0, 255, 0);
                    break;
                }
                case CubeColor::BLUE: {
                    cv::inRange(hsv_frame, get_lower_bound("blue"), get_upper_bound("blue"), mask);
                    bbox_color = cv::Scalar(255, 0, 0);
                    break;
                }
                case CubeColor::WHITE: {
                    cv::inRange(hsv_frame, get_lower_bound("white"), get_upper_bound("white"), mask);
                    bbox_color = cv::Scalar(255, 255, 255);
                    break;
                }
            }

            cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
            cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);

            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            geometry_msgs::msg::Point point_msg;
            point_msg.z = 0;
            double max_area = 0.0;
            cv::Point2f center(-1.0, -1.0);

            std::vector<cv::Point> best_contour;
            double current_min_area = this->get_parameter("min_area").as_double();

            for (const auto& contour : contours) {
                double area = cv::contourArea(contour);
                if (area > current_min_area  && area > max_area) {
                    max_area = area;
                    best_contour = contour;
                    cv::Moments M = cv::moments(contour);
                    if(M.m00 > 0){
                        center.x = M.m10 / M.m00;
                        center.y = M.m01 / M.m00;
                    }
                }
            }
            if (max_area > 0.0) {
                point_msg.x = center.x;
                point_msg.y = center.y;

                cv::Rect bounding_box = cv::boundingRect(best_contour);
                cv::rectangle(draw_frame, bounding_box, bbox_color, 2);
                cv::circle(draw_frame, center, 2, bbox_color, -1);
            } else {
                point_msg.x = std::numeric_limits<double>::lowest();
                point_msg.y = std::numeric_limits<double>::lowest();
            }
            publisher->publish(point_msg);  
        }

        cv::Scalar get_lower_bound(const std::string& prefix) {
            return cv::Scalar(
                this->get_parameter(prefix + ".h_min").as_int(),
                this->get_parameter(prefix + ".s_min").as_int(),
                this->get_parameter(prefix + ".v_min").as_int()
            );
        }

        cv::Scalar get_upper_bound(const std::string& prefix) {
            return cv::Scalar(
                this->get_parameter(prefix + ".h_max").as_int(),
                this->get_parameter(prefix + ".s_max").as_int(),
                this->get_parameter(prefix + ".v_max").as_int()
            );
        }

    public:
        CubeDetector(const rclcpp::NodeOptions& options) : Node("cube_detector", options),
        pub_red_(this->create_publisher<geometry_msgs::msg::Point>("cube_detector/red_cube/position_on_frame", rclcpp::QoS(10))),
        pub_green_(this->create_publisher<geometry_msgs::msg::Point>("cube_detector/green_cube/position_on_frame", rclcpp::QoS(10))),
        pub_blue_(this->create_publisher<geometry_msgs::msg::Point>("cube_detector/blue_cube/position_on_frame", rclcpp::QoS(10))),
        pub_white_(this->create_publisher<geometry_msgs::msg::Point>("cube_detector/white_cube/position_on_frame", rclcpp::QoS(10))),
        sub_image_(this->create_subscription<sensor_msgs::msg::Image>("/zed/zed_node/left_raw/image_raw_color", rclcpp::QoS(10), std::bind(&CubeDetector::image_callback, this, std::placeholders::_1))),
        pub_image_(this->create_publisher<sensor_msgs::msg::Image>("cube_detector/detected_cubes/image", rclcpp::QoS(10))) {
            // parametry pod testy
            this->declare_parameter("min_area", 300.0);
            auto declare_hsv_params = [this](const std::string& prefix, int h_min, int s_min, int v_min, int h_max, int s_max, int v_max) {
                this->declare_parameter(prefix + ".h_min", h_min);
                this->declare_parameter(prefix + ".s_min", s_min);
                this->declare_parameter(prefix + ".v_min", v_min);
                this->declare_parameter(prefix + ".h_max", h_max);
                this->declare_parameter(prefix + ".s_max", s_max);
                this->declare_parameter(prefix + ".v_max", v_max);
            };
            declare_hsv_params("green", 50, 100, 50, 110, 255, 255);
            declare_hsv_params("blue", 95, 100, 50, 110, 255, 255);
            declare_hsv_params("white", 0, 0, 180, 180, 40, 255);
            this->declare_parameter("red.h_min1", 0);
            this->declare_parameter("red.h_max1", 15);
            this->declare_parameter("red.h_min2", 170);
            this->declare_parameter("red.h_max2", 180);
            this->declare_parameter("red.s_min", 150);
            this->declare_parameter("red.s_max", 255);
            this->declare_parameter("red.v_min", 100);
            this->declare_parameter("red.v_max", 255);


            RCLCPP_INFO(this->get_logger(), "Node cube_detector was initialized");
        }
    };
}



#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(cube_detector::CubeDetector)


