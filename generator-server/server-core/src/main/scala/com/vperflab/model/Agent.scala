package com.vperflab.model

import net.liftweb.mongodb.record.{MongoMetaRecord, MongoRecord}
import net.liftweb.mongodb.record.field.ObjectIdPk
import net.liftweb.record.field.{DateTimeField, StringField}
import net.liftweb.record.LifecycleCallbacks
import java.util.{Calendar, Date}
import net.liftweb.util.Helpers

class Agent extends MongoRecord[Agent] with ObjectIdPk[Agent] {
  def meta = Agent
  object hostName extends StringField(this, 70)

  object updatedAt extends DateTimeField(this) with LifecycleCallbacks {
    implicit def date2Calendar(date: Date): Calendar = {
      val cal = Calendar.getInstance()
      cal.setTime(date)
      cal
    }

    override def defaultValue = Helpers.now

    override def beforeSave() {
      super.beforeSave
      this.set(Helpers.now)
    }
  }
}

object Agent extends Agent with MongoMetaRecord[Agent]{
}