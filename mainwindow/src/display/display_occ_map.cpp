
/*
 * @Author: chengyang chengyangkj@outlook.com
 * @Date: 2023-03-28 10:21:04
 * @LastEditors: chengyangkj chengyangkj@qq.com
 * @LastEditTime: 2023-10-15 02:49:39
 * @FilePath: ////src/display/robot_map.cpp
 */
#include <algorithm>
#include <iostream>

#include "display/display_occ_map.h"
namespace Display {
DisplayOccMap::DisplayOccMap(const std::string &display_type,
                             const int &z_value, std::string parent_name)
    : VirtualDisplay(display_type, z_value, parent_name) {
  SetMoveEnable(true);
}
bool DisplayOccMap::UpdateData(const std::any &data) {
  try {
    if (data.type() == typeid(OccupancyMap)) {
      map_data_ = std::any_cast<OccupancyMap>(data);
    }

  } catch (const std::bad_any_cast &e) {
    std::cout << e.what() << '\n';
    return false;
  }

  ParseOccupyMap();
  SetBoundingRect(QRectF(0, 0, map_image_.width(), map_image_.height()));
  update();
  // std::cout << "map update calling:" << map_image_.width() << " "
  //           << map_image_.height() << std::endl;
  return true;
}
bool DisplayOccMap::SetDisplayConfig(const std::string &config_name,
                                     const std::any &config_data) {
  if (config_name == "SubMapValue") {
    GetAnyData(double, config_data, sub_map_value_);
  } else if (config_name == "RobotPose") {
    // using type = Eigen::Vector3f;
    GetAnyData(Eigen::Vector3f, config_data, sub_map_center_pose_);
  } else {
    return false;
  }
  update();
  return true;
}
void DisplayOccMap::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget) {
  // //按照比例裁减地图
  // if (sub_map_value_ != 1) {
  //   //以机器人为中心按比例进行裁剪
  //   int width = std::min(map_data_.Cols(), map_data_.Rows());
  //   width *= sub_map_value_;
  //   int top_left_x = sub_map_center_pose_[0] - width / 2.0;
  //   int top_left_y = sub_map_center_pose_[1] - width / 2.0;
  //   top_left_x = top_left_x < 0 ? 0 : top_left_x;
  //   top_left_y = top_left_y < 0 ? 0 : top_left_y;
  //   int bottom_right_x = sub_map_center_pose_[0] + width / 2.0;
  //   int bottom_right_y = sub_map_center_pose_[1] + width / 2.0;
  //   bottom_right_x =
  //       bottom_right_x > map_data_.Rows() ? map_data_.Rows() :
  //       bottom_right_x;
  //   bottom_right_y =
  //       bottom_right_y > map_data_.Cols() ? map_data_.Cols() :
  //       bottom_right_y;
  //   sub_image = map_image_.copy(QRect(QPoint(top_left_x, top_left_y),
  //                                     QPoint(bottom_right_x,
  //                                     bottom_right_y)));
  //   draw_x = top_left_x;
  //   draw_y = top_left_y;
  // }
  painter->drawImage(0, 0, map_image_);
}
void DisplayOccMap::ParseOccupyMap() {
  // Eigen::matrix 坐标系与QImage坐标系不同,这里行列反着遍历
  map_image_ = QImage(map_data_.Cols(), map_data_.Rows(), QImage::Format_RGB32);
  QVector<QPointF> points;
  //QImage坐标系
  // **************x
  // *
  // *
  // *
  // y

  for (int i = 0; i < map_data_.Cols(); i++)
    for (int j = 0; j < map_data_.Rows(); j++) {
      double map_value = map_data_(j, i);
      QColor color;
      if (map_value > 0) {
        color = Qt::black;  // black
      } else if (map_value < 0) {
        color = Qt::gray;  // gray
      } else {
        color = Qt::white;  // white
      }
      map_image_.setPixel(i, j, color.rgb());
    }
}
void DisplayOccMap::EraseMapRange(const QPointF &pose, double range) {
  float x = pose.x();
  float y = pose.y();
  // 确保传入的坐标在图像范围内
  if (x < 0 || x >= map_image_.width() || y < 0 || y >= map_image_.height()) {
    return;
  }
  // 计算擦除范围的矩形区域
  int left = qMax(0, static_cast<int>(x - range));
  int top = qMax(0, static_cast<int>(y - range));
  int right = qMin(map_image_.width() - 1, static_cast<int>(x + range));
  int bottom = qMin(map_image_.height() - 1, static_cast<int>(y + range));

  // 循环遍历范围内的像素点，将其颜色设置为透明
  for (int i = left; i <= right; ++i) {
    for (int j = top; j <= bottom; ++j) {
      map_image_.setPixelColor(i, j, Qt::white);
    }
  }
  update();
}
OccupancyMap DisplayOccMap::GetOccupancyMap() {
  OccupancyMap map = map_data_;
  for (int i = 0; i < map_image_.width(); i++)
    for (int j = 0; j < map_image_.height(); j++) {
      QRgb pixelValue = map_image_.pixel(i, j);  // (x, y) 是指定位置的坐标
      if (pixelValue == QColor(Qt::black).rgb()) {
        map(j, i) = 100;
      } else if (pixelValue == QColor(Qt::gray).rgb()) {
        map(j, i) = -1;
      } else {
        map(j, i) = 0;
      }
    }

  return map;
}
void DisplayOccMap::StartDrawLine(const QPointF &pose) {
  line_start_pose_ = pose;
}
void DisplayOccMap::EndDrawLine(const QPointF &pose, bool is_draw) {
  if (!is_draw_line_) {
    line_tmp_image_ = map_image_;
    is_draw_line_ = true;
  }
  map_image_ = line_tmp_image_;
  QPainter painter(&map_image_);
  painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.drawLine(line_start_pose_, pose);
  if (is_draw) {
    //结束绘制
    is_draw_line_ = false;
  }
  update();
}
void DisplayOccMap::DrawPoint(const QPointF &point) {
  QPainter painter(&map_image_);
  painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.drawPoint(point);
  update();
}
}  // namespace Display