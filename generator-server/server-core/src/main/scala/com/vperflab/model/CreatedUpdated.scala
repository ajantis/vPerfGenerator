package com.vperflab.model

import net.liftweb.record.field.DateTimeField
import net.liftweb.record.LifecycleCallbacks
import java.util.{Calendar, Date}
import net.liftweb.util.Helpers
import net.liftweb.mongodb.record.MongoRecord

/**
 * Copyright iFunSoftware 2012
 * @author Dmitry Ivanov
 */
trait CreatedUpdated[T <: MongoRecord[T]]{ this: T with MongoRecord[T] =>

  implicit def date2Calendar(date: Date): Calendar = {
    val cal = Calendar.getInstance()
    cal.setTime(date)
    cal
  }

  object updatedAt extends DateTimeField[T](this) with LifecycleCallbacks {
    override def defaultValue = Helpers.now
    override def beforeSave {
      super.beforeSave
      this.set(Helpers.now)
    }
  }

  object createdAt extends DateTimeField[T](this) with LifecycleCallbacks {
    override def defaultValue = Helpers.now
    override def beforeSave {
      super.beforeSave
      this.set(Helpers.now)
    }
  }
}