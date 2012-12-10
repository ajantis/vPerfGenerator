package com.vperflab.web

import javax.servlet.{ServletContextEvent, ServletContextListener}
import org.slf4j.bridge.SLF4JBridgeHandler

class JulRedirectListener extends ServletContextListener {
  override def contextInitialized(event: ServletContextEvent) {
    SLF4JBridgeHandler.install()
  }

  override def contextDestroyed(event: ServletContextEvent) {
    SLF4JBridgeHandler.uninstall()
  }
}