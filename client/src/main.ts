import { createApp } from 'vue';
import App from '@/App.vue';
import Button from 'primevue/button';
import Card from 'primevue/card';
import Carousel from 'primevue/carousel';
import Chip from 'primevue/chip';
import ConfirmPopup from 'primevue/confirmpopup';
import InlineMessage from 'primevue/inlinemessage';
import InputSwitch from 'primevue/inputswitch';
import Knob from 'primevue/knob';
import Panel from 'primevue/panel';
import PrimeVue from 'primevue/config';
import TabPanel from 'primevue/tabpanel';
import TabView from 'primevue/tabview';
import Timeline from 'primevue/timeline';
import Tooltip from 'primevue/tooltip';

import ConfirmationService from 'primevue/confirmationservice';

import '@/assets/css/theme.css';
import 'primevue/resources/primevue.min.css';
import 'primeflex/primeflex.css';
import 'primeicons/primeicons.css';

const app = createApp(App);

app.component('Button', Button);
app.component('Card', Card);
app.component('Carousel', Carousel);
app.component('Chip', Chip);
app.component('ConfirmPopup', ConfirmPopup);
app.component('InlineMessage', InlineMessage);
app.component('InputSwitch', InputSwitch);
app.component('Knob', Knob);
app.component('Panel', Panel);
app.component('TabPanel', TabPanel);
app.component('TabView', TabView);
app.component('Timeline', Timeline);

app.directive('tooltip', Tooltip);

app.use(PrimeVue, { ripple: true });
app.use(ConfirmationService);

app.mount('#app');
