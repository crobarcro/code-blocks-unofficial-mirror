@echo off
if exist .\%output_dir% goto exists
mkdir %output_dir%
goto process
:exists
echo O | del .\%output_dir%\*
:process

rem htlatex %name%.tex html "" -d%output_dir% "--interaction=nonstopmode"
rem htlatex %name%.tex "html,3,sections+,info,next,refcaption,fn-in" -utf8 -d%output_dir% "--interaction=nonstopmode"
rem htlatex %name%.tex "configfiles/html,html,3,sections+,info,next,refcaption,fn-in" -utf8 -d%output_dir% "--interaction=nonstopmode"
htlatex %name%.tex "configfiles/htmlweb,html,pic-tabular,pic-longtable" -utf8 -d%output_dir% "--interaction=nonstopmode"

:clean
del %name%.4tc  > nul
del %name%.4ct  > nul
del %name%.tmp  > nul
del %name%.xref > nul
del %name%.idv  > nul
del %name%.lg   > nul
del %name%.html > nul
del %name%.css  > nul
del %name%.glo  > nul
del %name%.dep  > nul
del *.aux       > nul
del mystyles\*.aux       > nul
del %name%.foo  > nul
del %name%.out.*> nul
del %name%*.html> nul

Reset_png %lang%

goto end

:usage

echo Usage: %0 file.tex
echo *** Caution *** If there is a html directory it will be deleted.
echo. 
:end
pause
