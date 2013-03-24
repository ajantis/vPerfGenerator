package com.vperflab.model

import net.liftweb.mongodb.record.{BsonMetaRecord, BsonRecord, MongoMetaRecord, MongoRecord}
import net.liftweb.mongodb.record.field.{BsonRecordListField, MongoMapField, ObjectIdPk}
import net.liftweb.record.field.{BooleanField, StringField, LongField}

class Agent extends MongoRecord[Agent] with ObjectIdPk[Agent] with CreatedUpdated[Agent] {
  thisAgent =>

  def meta = Agent
  object hostName extends StringField(this, 70)
  object isActive extends BooleanField(this){
    override def defaultValue = false
  }

  object workloadTypes extends BsonRecordListField(this, WLTypeInfo)
  object threadPools extends BsonRecordListField(this, ThreadPoolInfo)

  import com.foursquare.rogue.Rogue._

  def workloadProfiles: List[WorkloadProfile] = WorkloadProfile.where(_.agent.obj.open_!.id eqs thisAgent.id.is).fetch()

  def url: String = meta.baseAgentsUrl + "/" + hostName
}

object Agent extends Agent with MongoMetaRecord[Agent]{
  val baseAgentsUrl = "/agents"
}

class WLTypeInfo private () extends BsonRecord[WLTypeInfo] {
  def meta = WLTypeInfo

  object name extends StringField(this, 256)
  object module extends StringField(this, 256)
  object path extends StringField(this, 1024)

  object params extends BsonRecordListField(this, WorkloadParamInfo)
}

object WLTypeInfo extends WLTypeInfo with BsonMetaRecord[WLTypeInfo]

class WorkloadParamInfo private () extends BsonRecord[WorkloadParamInfo]{
  def meta = WorkloadParamInfo

  object name extends StringField(this, 128)

  object description extends StringField(this, 1024)

  object additionalData extends MongoMapField[WorkloadParamInfo, String](this)

}

object WorkloadParamInfo extends WorkloadParamInfo with BsonMetaRecord[WorkloadParamInfo]

class ThreadPoolInfo private () extends BsonRecord[ThreadPoolInfo] {
  def meta = ThreadPoolInfo
  
  object name extends StringField(this, 64)
  
  object numThreads extends LongField(this)
  object quantum extends LongField(this)
  object workloadCount extends LongField(this)
}

object ThreadPoolInfo extends ThreadPoolInfo with BsonMetaRecord[ThreadPoolInfo]