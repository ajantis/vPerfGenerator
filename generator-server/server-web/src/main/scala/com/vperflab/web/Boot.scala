package com.vperflab.web

import _root_.net.liftweb.util._
import _root_.net.liftweb.common._
import _root_.net.liftweb.http._
import _root_.net.liftweb.http.provider._
import _root_.net.liftweb.sitemap._
import _root_.net.liftweb.sitemap.Loc.LocGroup
import _root_.net.liftweb.mapper.{DB, Schemifier, DefaultConnectionIdentifier, StandardDBVendor}
import _root_.net.liftweb.widgets.logchanger._
import net.liftweb.widgets.flot._
import net.liftweb.http.ResourceServer
import com.vperflab.model.{Iteration, Experiment, 
			IterationExecution, Execution, Agent}
import model.User
import snippet.LogLevel
import net.liftweb.sitemap.Loc.Hidden

/**
 * A class that's instantiated early and run.  It allows the application
 * to modify lift's environment
 */
class Boot extends Bootable {

  def boot {
    if (!DB.jndiJdbcConnAvailable_?) {
      val vendor =
        new StandardDBVendor(Props.get("db.driver") openOr "org.h2.Driver",
          Props.get("db.url") openOr
            "jdbc:h2:lift_proto.db;AUTO_SERVER=TRUE",
          Props.get("db.user"), Props.get("db.password"))

      LiftRules.unloadHooks.append(vendor.closeAllConnections_! _)

      DB.defineConnectionManager(DefaultConnectionIdentifier, vendor)
    }

    // where to search snippet
    val snippetPackages = List("com.vperflab.web")

    snippetPackages.foreach(LiftRules.addToPackages(_))

    Schemifier.schemify(true, Schemifier.infoF _, User, Experiment, Iteration, 
        IterationExecution, Execution, Agent)

    // Build SiteMap
    def sitemap() = SiteMap(
      // Menus on bottom bar (main menu)
      Menu("Welcome") / "index" >> LocGroup("mainMenu") , // >> User.AddUserMenusAfter,
      Menu("Agents") / "agents" / "index" >> LocGroup("mainMenu"),
      Menu("Experiments") / "experiments" / "index" >> LocGroup("mainMenu"),
      Menu("Profiles") / "profiles" / "index" >> LocGroup("mainMenu"),
      Menu("Monitoring") / "monitoring" >> LocGroup("mainMenu"),
      
      // Menus on top bar (helper menu)
      Menu("Help") / "help" >> LocGroup("topMenu"),
      Menu("About") / "about" >> LocGroup("topMenu"),
      
      LogLevel.menu // adding a menu for log level changer snippet page. By default it's /loglevel/change
    )

    LiftRules.setSiteMapFunc(() => sitemap())

    /*
     * Show the spinny image when an Ajax call starts
     */
    LiftRules.ajaxStart =
      Full(() => LiftRules.jsArtifacts.show("ajax-loader").cmd)

    /*
     * Make the spinny image go away when it ends
     */
    LiftRules.ajaxEnd =
      Full(() => LiftRules.jsArtifacts.hide("ajax-loader").cmd)

    LiftRules.early.append(makeUtf8)

    LiftRules.loggedInTest = Full(() => User.loggedIn_?)
    
    LiftRules.statelessRewrite.prepend(NamedPF("ParticularExperimentRewrite") {
      case RewriteRequest(
      ParsePath("experiments" :: experimentId :: Nil , _, _,_), _, _) if (isNumeric(experimentId)) =>
        RewriteResponse(
          "experiments" ::  "view" :: Nil, Map("expId" -> experimentId)
        )
    })

    LiftRules.statelessRewrite.prepend(NamedPF("ParticularExecutionRewrite") {
      case RewriteRequest(
      ParsePath("experiments" :: "executions" :: executionId :: Nil , _, _,_), _, _) if (isNumeric(executionId)) =>
        RewriteResponse(
          "experiments" :: "executions" :: "view" :: Nil, Map("execId" -> executionId)
        )
    })

    Flot.init
    ResourceServer.allow({
      case "flot" :: "jquery.flot.stack.js" :: Nil => true
    })

    // instantiating of loglevel changer widget
    LogLevelChanger.init

    S.addAround(DB.buildLoanWrapper())
  }

  /**
   * Force the request to be UTF-8
   */
  private def makeUtf8(req: HTTPRequest) {
    req.setCharacterEncoding("UTF-8")
  }

  private def isNumeric(str: String) = {
    str.matches("\\d+")
  }
}