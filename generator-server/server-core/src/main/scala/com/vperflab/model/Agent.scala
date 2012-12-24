package com.vperflab.model

import net.liftweb.mongodb.record.{MongoMetaRecord, MongoRecord}
import net.liftweb.mongodb.record.field.ObjectIdPk
import net.liftweb.record.field.{BooleanField, StringField}

class Agent extends MongoRecord[Agent] with ObjectIdPk[Agent] with CreatedUpdated[Agent] {
  thisAgent =>

  def meta = Agent
  object hostName extends StringField(this, 70)
  object isActive extends BooleanField(this){
    override def defaultValue = false
  }

  import com.foursquare.rogue.Rogue._

  def workloadProfiles: List[WorkloadProfile] = WorkloadProfile.where(_.agent.obj.open_!.id eqs thisAgent.id.is).fetch()
}

object Agent extends Agent with MongoMetaRecord[Agent]