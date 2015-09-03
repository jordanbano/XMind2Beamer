@echo off

IF "%~1"=="" GOTO endparse
IF NOT "%~x1"==".xmind" GOTO endparse

:: create a copy of the .xmind file which is in fact a .zip file
echo Move the .xmind file into a temp file
copy %1 Temp.zip > log

:: unrar it
echo Unzip it
"C:\Program Files (x86)\WinRAR\WinRAR.exe" x Temp.zip * .\%~n1\ -o+ > log
del Temp.zip

:: force compilation
IF "%~2"=="-c" GOTO compil

IF EXIST Xmind2Beamer.exe GOTO tex

:compil
echo Compile the application
cl /EHsc /I c:\local\boost_1_55_0 Xmind2Beamer.cpp


:tex
:: launch the traducer
echo Generate .tex file
Xmind2Beamer.exe %~n1 > log

cd .\%~n1\

:: remove the white background of the images 
echo Convert pictures (transparent background)
cd .\attachments\
for /r %%i in (*) do convert "%%i" -fuzz 10%% -transparent white "%%~ni"%.png
cd ..

:: compile the .tex file
echo Generate the .pdf (it can take some times)
"C:\Program Files\MiKTeX 2.9\miktex\bin\x64\texify.exe" --pdf --quiet .\Temp.tex > log
:: save the .pdf file
copy .\Temp.pdf ..\%~n1.pdf > log

cd ..

echo Clear the temp files
:: remove the temporary files
::rmdir /Q /S %~n1\


:endparse