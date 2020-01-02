@echo off
set lang=fr
set output_dir=htmlweb_%lang%
if [%1]==[] goto nomdef
set name=%~n1
goto suite
:nomdef
rem set name=00_test_fr
set name=main_codeblocks_%lang%
:suite
.\00_ToHtml.cmd
