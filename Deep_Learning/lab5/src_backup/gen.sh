head -2 ai.js > ai.head
tail -n +4 ai.js > ai.body
echo "this.w = " > ai.w

cat ai.head ai.w weights.txt ai.body > ai.out.js
rm ai.head ai.w ai.body
