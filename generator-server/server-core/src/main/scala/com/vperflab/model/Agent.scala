package com.vperflab.model

import net.liftweb.mongodb.record.{BsonMetaRecord, BsonRecord, MongoMetaRecord, MongoRecord}
import net.liftweb.mongodb.record.field.{BsonRecordListField, MongoMapField, ObjectIdPk}
import net.liftweb.record.field.{BooleanField, StringField}

class Agent extends MongoRecord[Agent] with ObjectIdPk[Agent] with CreatedUpdated[Agent] {
  thisAgent =>

  def meta = Agent
  object hostName extends StringField(this, 70)
  object isActive extends BooleanField(this){
    override def defaultValue = false
  }

  object modules extends BsonRecordListField(this, ModuleInfo)

  import com.foursquare.rogue.Rogue._

  def workloadProfiles: List[WorkloadProfile] = WorkloadProfile.where(_.agent.obj.open_!.id eqs thisAgent.id.is).fetch()

  def url: String = meta.baseAgentsUrl + "/" + hostName
}

object Agent extends Agent with MongoMetaRecord[Agent]{
  val baseAgentsUrl = "/agents"
}

class ModuleInfo private () extends BsonRecord[ModuleInfo] {
  def meta = ModuleInfo

  object name extends StringField(this, 256)
  object path extends StringField(this, 1024)

  object params extends BsonRecordListField(this, WorkloadParamInfo)
}

object ModuleInfo extends ModuleInfo with BsonMetaRecord[ModuleInfo]

class WorkloadParamInfo private () extends BsonRecord[WorkloadParamInfo]{
  def meta = WorkloadParamInfo

  object name extends StringField(this, 128)

  object description extends StringField(this, 1024)

  object additionalData extends MongoMapField[WorkloadParamInfo, String](this)

}

object WorkloadParamInfo extends WorkloadParamInfo with BsonMetaRecord[WorkloadParamInfo]