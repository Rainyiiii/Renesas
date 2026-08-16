@echo off
cd /d "%~dp0.."
.venv\Scripts\python.exe -u train.py --data data\my_first_project_v1_192.data --npu-friendly 1>train_my_first_project_v1.log 2>train_my_first_project_v1.err.log
