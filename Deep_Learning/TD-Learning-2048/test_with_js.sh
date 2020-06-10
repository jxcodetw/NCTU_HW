head -2 ai.template.js > ai.head
tail -n +5 ai.template.js > ai.body

cat ai.head ai.w ai.ntuple ai.body > ai.out.js
rm ai.head ai.body

echo "open http://murgo.github.io/2048/?script=http://localhost:8000/ai.out.js&seed=9527"
python3 -m http.server