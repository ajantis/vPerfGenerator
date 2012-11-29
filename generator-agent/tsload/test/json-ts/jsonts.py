import json

msgId = 0

def getMessageId():
    global msgId
    msgId += 1
    return msgId

def createCommandMsg(cmd, msg):
    return {"id": getMessageId(),
            "cmd": cmd,
            "msg": msg}

def serializeMsg(msg):
    return json.dumps(msg) + '\0'