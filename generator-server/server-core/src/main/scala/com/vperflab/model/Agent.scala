package com.vperflab.model

import net.liftweb.mapper._

class Agent extends LongKeyedMapper[Agent] with IdPK {
  def getSingleton = Agent
  
  object hostName extends MappedString(this, 70)
}

object Agent extends Agent with LongKeyedMetaMapper[Agent] {
  override def dbTableName = "agents"
}