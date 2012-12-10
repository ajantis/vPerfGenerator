package com.vperflab.web.snippet

import net.liftweb.util.Props
import net.liftweb.sitemap.Loc
import net.liftweb.widgets.logchanger._
import com.vperflab.web.model.User

/**
 * @author Dmitry Ivanov
 * iFunSoftware
 */
object LogLevel extends LogLevelChanger with LogbackLoggingBackend {

  override
  def menuLocParams: List[Loc.AnyLocParam] =
    if (Props.productionMode)
      List(User.testSuperUser)
    else
      Nil
}