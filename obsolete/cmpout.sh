
FILE=hardcore/crash1

echo "version 4"
#g4tool-0.4 -d v -b <$FILE >/dev/null 2>t1
echo "version current"
#g4tool -d v -b <$FILE >/dev/null 2>t2

echo "diffing"
diff t1 t2 >td
