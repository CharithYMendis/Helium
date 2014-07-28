
@echo off
Setlocal EnableDelayedExpansion

set var=charith;
set var_app=

for %%rt in (%var%) do (
	set var_app=!var_app!;%%rt_test
)

for %%s in (%var%) do echo %%s
for %%s in (%var_app%) do echo %%s
