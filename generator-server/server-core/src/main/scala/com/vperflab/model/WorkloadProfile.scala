package com.vperflab.model

import net.liftweb.mongodb.record.{MongoMetaRecord, MongoRecord}
import net.liftweb.mongodb.record.field.{ObjectIdRefField, ObjectIdPk, MongoMapField}
import net.liftweb.record.field.StringField
import net.liftweb.common.Full

/**
 * Copyright iFunSoftware 2012
 * @author Dmitry Ivanov
 */
class WorkloadProfile extends MongoRecord[WorkloadProfile] with ObjectIdPk[WorkloadProfile] with CreatedUpdated[WorkloadProfile] {
  def meta = WorkloadProfile

  object params extends MongoMapField(this)
  object name extends StringField(this, 70)
  object agent extends ObjectIdRefField(this, Agent) {
    override def options = Agent.findAll.map(agent => (Full(agent.id.is), agent.hostName.is))
  }
}

object WorkloadProfile extends WorkloadProfile with MongoMetaRecord[WorkloadProfile]