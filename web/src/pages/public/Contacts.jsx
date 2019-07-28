import React from 'react';
import { connect } from 'react-redux';

const Contacts = props => (
    <div>
        <p>Вы можете связаться с нами следующим способом:</p>
        <ul>
            <li>Электронная почта: <a href="mailto:contact@iv-soft.ru">contact@iv-soft.ru</a></li>
            <li>Телеграм/WhatsApp: @lboss75</li>
        </ul>
    </div>
);

export default connect()(Contacts);
