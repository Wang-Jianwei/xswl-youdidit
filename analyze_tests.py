#!/usr/bin/env python3

# xswl-youdidit 测试结果分析脚本
# 解析测试输出并生成详细报告
# 用法: python3 analyze_tests.py [构建目录]

import os
import sys
import subprocess
import json
from pathlib import Path
from datetime import datetime

class TestAnalyzer:
    def __init__(self, build_dir):
        self.build_dir = Path(build_dir)
        self.results = {
            'timestamp': datetime.now().isoformat(),
            'unit_tests': {},
            'integration_tests': {},
            'examples': {},
            'summary': {}
        }
    
    def run_test(self, test_path, test_name):
        """运行单个测试并记录结果"""
        try:
            result = subprocess.run(
                [str(test_path)],
                capture_output=True,
                text=True,
                timeout=30
            )
            return {
                'name': test_name,
                'status': 'PASSED' if result.returncode == 0 else 'FAILED',
                'returncode': result.returncode,
                'stdout': result.stdout,
                'stderr': result.stderr
            }
        except subprocess.TimeoutExpired:
            return {
                'name': test_name,
                'status': 'TIMEOUT',
                'returncode': -1,
                'stdout': '',
                'stderr': 'Test timeout after 30 seconds'
            }
        except Exception as e:
            return {
                'name': test_name,
                'status': 'ERROR',
                'returncode': -1,
                'stdout': '',
                'stderr': str(e)
            }
    
    def run_unit_tests(self):
        """运行所有单元测试"""
        print("🧪 运行单元测试...")
        unit_tests = [
            'test_types', 'test_task', 'test_task_builder',
            'test_claimer', 'test_task_platform', 'test_thread_safety', 'test_web'
        ]
        
        for test_name in unit_tests:
            test_path = self.build_dir / 'tests' / test_name
            if test_path.exists():
                result = self.run_test(test_path, test_name)
                self.results['unit_tests'][test_name] = result
                status_icon = '✅' if result['status'] == 'PASSED' else '❌'
                print(f"  {status_icon} {test_name}: {result['status']}")
            else:
                print(f"  ⚠️  {test_name}: 未找到")
    
    def run_integration_tests(self):
        """运行所有集成测试"""
        print("🔗 运行集成测试...")
        int_tests = [
            'integration_test_workflow',
            'integration_test_web_api'
        ]
        
        for test_name in int_tests:
            test_path = self.build_dir / 'tests' / test_name
            if test_path.exists():
                result = self.run_test(test_path, test_name)
                self.results['integration_tests'][test_name] = result
                status_icon = '✅' if result['status'] == 'PASSED' else '❌'
                print(f"  {status_icon} {test_name}: {result['status']}")
            else:
                print(f"  ⚠️  {test_name}: 未找到")
    
    def run_examples(self):
        """运行示例程序"""
        print("📚 运行示例程序...")
        examples = [
            'example_basic_usage',
            'example_multi_claimer',
            'example_web_monitoring'
        ]
        
        for example_name in examples:
            example_path = self.build_dir / 'examples' / example_name
            if example_path.exists():
                result = self.run_test(example_path, example_name)
                self.results['examples'][example_name] = result
                status_icon = '✅' if result['status'] == 'PASSED' else '❌'
                print(f"  {status_icon} {example_name}: {result['status']}")
            else:
                print(f"  ⚠️  {example_name}: 未找到")
    
    def generate_summary(self):
        """生成测试总结"""
        total_unit = len(self.results['unit_tests'])
        passed_unit = sum(1 for r in self.results['unit_tests'].values() if r['status'] == 'PASSED')
        
        total_int = len(self.results['integration_tests'])
        passed_int = sum(1 for r in self.results['integration_tests'].values() if r['status'] == 'PASSED')
        
        total_examples = len(self.results['examples'])
        passed_examples = sum(1 for r in self.results['examples'].values() if r['status'] == 'PASSED')
        
        self.results['summary'] = {
            'total_tests': total_unit + total_int,
            'passed_tests': passed_unit + passed_int,
            'unit_tests': {'total': total_unit, 'passed': passed_unit},
            'integration_tests': {'total': total_int, 'passed': passed_int},
            'examples': {'total': total_examples, 'passed': passed_examples},
            'overall_status': 'PASSED' if (passed_unit == total_unit and passed_int == total_int) else 'FAILED'
        }
    
    def print_report(self):
        """打印测试报告"""
        summary = self.results['summary']
        
        print("\n" + "="*60)
        print("📊 测试报告")
        print("="*60)
        print(f"测试时间: {self.results['timestamp']}")
        print()
        print(f"总体状态: {summary['overall_status']}")
        print(f"测试总数: {summary['total_tests']}")
        print(f"通过数: {summary['passed_tests']}")
        print(f"失败数: {summary['total_tests'] - summary['passed_tests']}")
        print()
        print(f"单元测试: {summary['unit_tests']['passed']}/{summary['unit_tests']['total']} 通过")
        print(f"集成测试: {summary['integration_tests']['passed']}/{summary['integration_tests']['total']} 通过")
        print(f"示例程序: {summary['examples']['passed']}/{summary['examples']['total']} 通过")
        print("="*60 + "\n")
    
    def export_json(self, output_file):
        """导出 JSON 格式的测试结果"""
        with open(output_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        print(f"✅ 测试结果已保存至: {output_file}")
    
    def run_all(self):
        """运行所有测试"""
        print("开始运行测试套件...\n")
        self.run_unit_tests()
        print()
        self.run_integration_tests()
        print()
        self.run_examples()
        print()
        self.generate_summary()
        self.print_report()

def main():
    if len(sys.argv) > 1:
        build_dir = sys.argv[1]
    else:
        # 默认使用当前目录的 build 子目录
        build_dir = './build'
    
    build_path = Path(build_dir)
    if not build_path.exists():
        print(f"❌ 错误: 构建目录不存在: {build_dir}")
        sys.exit(1)
    
    analyzer = TestAnalyzer(build_path)
    analyzer.run_all()
    
    # 导出 JSON 结果
    json_output = build_path / 'test_results.json'
    analyzer.export_json(json_output)

if __name__ == '__main__':
    main()
