@echo off

echo %~1

for %%s in (%~1) do (
	echo %%s
)