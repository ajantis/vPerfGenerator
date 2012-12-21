package com.vperflab.web.model

import com.vperflab.web.lib.{MegaProtoUser, MetaMegaProtoUser}
import net.liftweb.common.Full
import java.util.{Calendar, Date}
import net.liftweb.record.field.DateTimeField
import net.liftweb.record.LifecycleCallbacks
import net.liftweb.util.Helpers

/**
 * The singleton that has methods for accessing the database
 */
object User extends User with MetaMegaProtoUser[User] {
  //  override def dbTableName = "users" // define the DB table name

  override def screenWrap = Full(<lift:surround with="default" at="content">
      <lift:bind /></lift:surround>)
  // define the order fields will appear in forms and output
  //  override def fieldOrder = List(id, firstName, lastName, email,
  //  locale, timezone, password)

  // comment this line out to require email validations
  override def skipEmailValidation = true
}

/**
 * An O-R mapped "User" class that includes first name, last name, password and we add a "Personal Essay" to it
 */
class User extends MegaProtoUser[User] {
  def meta = User // what's the "meta" server

  implicit def date2Calendar(date: Date): Calendar = {
    val cal = Calendar.getInstance()
    cal.setTime(date)
    cal
  }

  object updatedAt extends DateTimeField(this) with LifecycleCallbacks {
    override def defaultValue = {
      Helpers.now
    }
    override def beforeSave() {
      super.beforeSave
      this.set(Helpers.now)
    }
  }
}

