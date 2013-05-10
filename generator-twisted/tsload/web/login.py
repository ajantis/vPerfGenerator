'''
Created on 10.05.2013

@author: myaut
'''

from tsload.web import TSWebAgent

from nevow import rend
from nevow import loaders
from nevow import inevow
from nevow import url
from nevow import tags as T

from nevow.util import Deferred

from formless import annotate
from formless import webform
from formless import configurable 

from zope.interface import implements

from tsload.web.main import MainPage


class ILoginForm(annotate.TypedInterface):
    def login(ctx = annotate.Context(),
        userName = annotate.String(required=True, requiredFailMessage='Please enter your name'),
        password = annotate.PasswordEntry(required=True, requiredFailMessage='Please enter your name')):
            pass
    annotate.autocallable(login)

class LoginFailurePage(MainPage):
    errorMessage = ''
    
    def render_content(self):
        return 
        
class LoginPage(MainPage):    
    implements(ILoginForm)
    
    def render_content(self, ctx, data):
        return loaders.xmlfile('webapp/login.html')
        
    def render_loginFailure(self, ctx, data):
        session = inevow.ISession(ctx)
        
        if getattr(session, 'loginFailure', ''):
            ret = T.div[
                     T.div(_class='alert alert-error')[session.loginFailure],
                     ]
            del session.loginFailure
            return ret
        
        return ''
    
    def render_loginForm(self, loginForm):        
        return webform.renderForms()
    
    def render_CSS(self, ctx, data):
        return file('webapp/css/login.css').read()
    
    def loginResponse(self, agent, session):
        if agent.connected == False:
            session.loginFailure = 'Connection Error: ' + agent.connectionError
            
            del session.agent
            del session.userName
            
            return ''
        else:
            return url.here.up()
    
    def login(self, ctx, userName, password):
        session = inevow.ISession(ctx)
        
        session.agent = TSWebAgent.createAgent('web', 'localhost', 9090)
        session.userName = userName
        
        d = Deferred()
        d.addCallback(self.loginResponse, session)
        
        session.agent.setEventListener(d)
        
        return d

        