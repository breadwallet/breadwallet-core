
if [ 2 != $# ]; then exit 0; fi

SRC_FILE=$1
TGT_FILE=$2

if [ -f "${SRC_FILE}"  ]; then
    cp "${SRC_FILE}" "${TGT_FILE}"
    chmod 644 "${TGT_FILE}"
    if [ ! -f "${TGT_FILE}" ]; then
	echo "Copy to  \\"${TGT_FILE}\\" failed."
    else
	echo "Copied \\"${SRC_FILE}\\" to \\"${TGT_FILE}\\""
    fi
else
    echo "Missing file ${SRC_FILE} - You must provide this file. (Created a default at '${TGT_FILE}')"
    rm -rf ${TGT_FILE}
    cat > ${TGT_FILE} <<EOF
[
  {
    "identifier": "ginger",
    "paperKey":   "ginger settle marine tissue robot crane night number ramp coast roast critic",
    "timestamp":  "2018-01-01",
    "network":    "testnet",
    "content":    "BTC, BCH, ETH"
  },

  {
    "identifier": "tape",
    "paperKey":   "tape argue fetch truck cattle quiz wide equal inform rabbit ranch educate",
    "timestamp":  "2018-04-02",
    "network":    "testnet",
    "content":    "(NO) BTC"
  },

  {
    "identifier": "infant",
    "paperKey":   "infant impulse panda donate sea  language champion art uphold stable water ice",
    "timestamp":  "2019-05-01",
    "network":    "testnet",
    "content":    "(NO?) BTC"
  },

  {
    "identifier": "vault",
    "paperKey":   "vault area cruel pull found hold collect current install  proud fluid isolate",
    "timestamp":  "2019-03-01",
    "network":    "testnet",
    "content":    "BTC"
  },

  {
    "identifier": "toe",
    "paperKey":   "toe web edge social worth borrow argue shield globe ritual fat upgrade",
    "timestamp":  "2018-01-01",
    "network":    "testnet",
    "content":    "BTC, ETH"
  },

  {
    "identifier": "general",
    "paperKey":   "general shaft mirror pave page talk basket crumble thrive gaze bamboo maid",
    "timestamp":  "2019-08-15",
    "network":    "testnet",
    "content":    "(Core - test transactions) BTC - ADDR-0: mzjmRwzABk67iPSrLys1ACDdGkuLcS6WQ4"
  },

  {
    "identifier": "blush",
    "paperKey":   "blush wear arctic fruit unique quantum because mammal entry country school curtain",
    "timestamp":  "2018-01-01",
    "network":    "testnet",
    "content":    "BCH"
  },

  {
    "identifier": "boring",
    "paperKey":   "boring head harsh green empty clip fatal typical found crane dinner timber",
    "timestamp":  "2018-04-26",
    "network":    "mainnet",
    "content":    "<COMPROMISED>"
  }
]
EOF
fi
