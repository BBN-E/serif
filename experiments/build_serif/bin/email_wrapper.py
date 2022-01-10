###
# Convenience module for sending a method call result by email.
###

import email.mime.text
import smtplib


def run_and_send(sender, recipient, subject_prefix, wrapped_method):
    """
    Executes the wrapped_method. If it throws an exception, that error
    message is sent via email to the recipient; otherwise the method's
    return value is sent as a string.

    Arguments:
    sender -- An email address that will send the message.
    recipient -- An email address that will receive the message.
    subject_prefix -- A short string that will be the message subject.
    wrapped_method -- A callable that takes no arguments and returns
    an object that can be converted to a Unicode string.
    """

    try:
        message_text = unicode(wrapped_method())
        subject = "%s SUCCESS" % subject_prefix
    except Exception as e:
        message_text = unicode(e)
        subject = "%s FAILURE" % subject_prefix
    finally:
        # Construct email message
        message = email.mime.text.MIMEText(message_text)
        message['Subject'] = subject
        message['From'] = sender
        message['To'] = recipient

        # Send email message
        smtp = smtplib.SMTP_SSL('smtp.bbn.com')
        smtp.sendmail(sender, [recipient], message.as_string())
        smtp.quit()
