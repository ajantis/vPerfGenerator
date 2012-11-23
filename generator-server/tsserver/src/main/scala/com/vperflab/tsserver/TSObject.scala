package com.vperflab.tsserver

abstract class TSObject

class TSHostInfo extends TSObject {
  var hostName: String = _
  
  def getHostName() : String = {
    return hostName;
  }
}