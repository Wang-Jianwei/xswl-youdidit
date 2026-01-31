/**
 * @file youdidit.hpp
 * @brief xswl-youdidit 主头文件
 * 
 * 包含所有核心公开接口，用户只需包含此文件即可使用完整功能
 */

#ifndef XSWL_YOUDIDIT_HPP
#define XSWL_YOUDIDIT_HPP

// 核心类型定义
#include <xswl/youdidit/core/types.hpp>

// 核心类
#include <xswl/youdidit/core/task.hpp>
#include <xswl/youdidit/core/task_builder.hpp>
#include <xswl/youdidit/core/claimer.hpp>
#include <xswl/youdidit/core/task_platform.hpp>

/**
 * @namespace xswl
 * @brief 顶级命名空间
 */
namespace xswl {

/**
 * @namespace xswl::youdidit
 * @brief xswl-youdidit 库的主命名空间
 * 
 * 包含任务管理、申领者管理、平台管理等核心功能
 */
namespace youdidit {

// 版本信息
constexpr const char* VERSION = "1.0.0";
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_HPP
