JDK = "$(JAVA_HOME)"
JDK= "C:\Program Files\Java\jdk-11.0.3"

java -version
javac -version
echo %JAVA_HOME%
echo %JDK_HOME%
set JAVA_HOME=C:\Program Files (x86)\Java\jdk1.8.0_212
set JDK_HOME=C:\Program Files (x86)\Java\jdk1.8.0_212
java -version
javac -version
echo %JAVA_HOME%
echo %JDK_HOME%

rem set CLASSPATH=C:\Users\Olivier\Downloads\notes-master-ffi\Bonjour\Clients\Java
rem set CLASSPATH="C:\Program Files (x86)\Java\jdk1.8.0_212\bin";.

javaw -jar BrowserApp.jar

rem java -verbose:gc,class,jni -cp ".\*"  -Djava.library.path="." -jar BrowserApp.jar

pause