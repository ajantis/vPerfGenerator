package com.vperflab.generator.web

import _root_.net.liftweb.util._
import _root_.net.liftweb.common._
import _root_.net.liftweb.http._
import _root_.net.liftweb.http.provider._
import _root_.net.liftweb.sitemap._
import _root_.net.liftweb.widgets.logchanger._
import model.User
import snippet.LogLevel
import net.liftweb.mongodb.{MongoHost, MongoAddress, DefaultMongoIdentifier, MongoDB}

/**
 * A class that's instantiated early and run.  It allows the application
 * to modify lift's environment
 */
class Boot extends Bootable {

  def boot() {
    MongoDB.defineDb(
      DefaultMongoIdentifier,
      MongoAddress(MongoHost( Props.get("mongodb.host") openOr "127.0.0.1"), Props.get("mongodb.name") openOr "lift_app")
    )

    // where to search snippet
    val snippetPackages = List("com.vperflab.generator.web")

    snippetPackages.foreach(LiftRules.addToPackages(_))


    // Build SiteMap
    def sitemap() = SiteMap(
      Menu("Home") / "index", // >> User.AddUserMenusAfter,
      LogLevel.menu // adding a menu for log level changer snippet page. By default it's /loglevel/change
    )

    LiftRules.setSiteMapFunc(() => User.sitemapMutator(sitemap()))

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

    // instantiating of loglevel changer widget
    LogLevelChanger.init
  }

  /**
   * Force the request to be UTF-8
   */
  private def makeUtf8(req: HTTPRequest) {
    req.setCharacterEncoding("UTF-8")
  }
}